#pragma once

#include <Foundation/Common.h>
#include <Scene/ISceneRenderer.h>

#include <Core/Utils.h>
#include <Renderer/DescriptorSet.h>
#include <Renderer/Pipeline.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/PipelineBarrier.h>
#include <Threading/JobSystem.h>
#include <Asset/AssetManager.h>
#include <Asset/AssetCompressor.h>
#include <Platform/Vulkan/VulkanCommon.h>
#include <DebugUtils/DebugRenderer.h>
#include <Scene/SceneRendererPrimitives.h>

namespace Omni {

	ISceneRenderer::ISceneRenderer(const SceneRendererSpecification& spec)
		: m_Specification(spec)
		, m_MaterialDataPool(this, 4096 * 256 /* 256Kb of data*/)
		, m_TextureIndexAllocator(VirtualMemoryBlock::Create(&g_PersistentAllocator, 4 * UINT16_MAX))
		, m_MeshResourcesBuffer(4096 * sizeof(GeometryMeshData)) // allow up to 4096 meshes be allocated at once
		, m_StorageImageIndexAllocator(VirtualMemoryBlock::Create(&g_PersistentAllocator, 4 * UINT16_MAX)) 
	{
		// Descriptor data
		{
			std::vector<DescriptorBinding> bindings;
			bindings.push_back({ 0, DescriptorBindingType::SAMPLED_IMAGE, UINT16_MAX, (uint64)DescriptorFlags::PARTIALLY_BOUND });
			bindings.push_back({ 1, DescriptorBindingType::STORAGE_IMAGE, 1, 0 });
			bindings.push_back({ 2, DescriptorBindingType::ACCELERATION_STRUCTURE, 1, 0 });
			bindings.push_back({ 3, DescriptorBindingType::STORAGE_IMAGE, 1, 0 });

			DescriptorSetSpecification global_set_spec = {};
			global_set_spec.bindings = std::move(bindings);

			for (int i = 0; i < Renderer::GetConfig().frames_in_flight; i++) {
				auto set = DescriptorSet::Create(&g_PersistentAllocator, global_set_spec);
				m_SceneDescriptorSet.push_back(set);
			}

			// Initialize render targets
			{
				ImageSpecification attachment_spec = ImageSpecification::Default();
				attachment_spec.usage = ImageUsage::RENDER_TARGET;
				attachment_spec.extent = Renderer::GetSwapchainImage()->GetSpecification().extent;
				attachment_spec.format = ImageFormat::RGBA64_HDR;
				OMNI_DEBUG_ONLY_CODE(attachment_spec.debug_name = "SceneRenderer output");
				for (int i = 0; i < Renderer::GetConfig().frames_in_flight; i++) {
					m_RendererOutputs.push_back(Image::Create(&g_PersistentAllocator, attachment_spec));
					m_SceneDescriptorSet[i]->Write(3, 0, m_RendererOutputs[i], nullptr);
				}

				attachment_spec.usage = ImageUsage::DEPTH_BUFFER;
				attachment_spec.format = ImageFormat::D32;

				for (int i = 0; i < Renderer::GetConfig().frames_in_flight; i++)
					m_DepthAttachments.push_back(Image::Create(&g_PersistentAllocator, attachment_spec));

			}

		}

		{
			// Initialize camera buffer
			DeviceBufferSpecification buffer_spec = {};
			buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
			buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
			buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
			buffer_spec.size = sizeof(ViewData) * Renderer::GetConfig().frames_in_flight;
			m_CameraDataBuffer = DeviceBuffer::Create(&g_PersistentAllocator, buffer_spec);
		}

		// Initialize nearest filtration sampler
		{
			ImageSamplerSpecification sampler_spec = {};
			sampler_spec.min_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.mag_filtering_mode = SamplerFilteringMode::NEAREST;
			sampler_spec.mipmap_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.address_mode = SamplerAddressMode::REPEAT;
			sampler_spec.min_lod = 0.0f;
			sampler_spec.max_lod = 1000.0f;
			sampler_spec.lod_bias = 0.0f;
			sampler_spec.anisotropic_filtering_level = m_Specification.anisotropic_filtering;

			m_SamplerNearest = ImageSampler::Create(&g_PersistentAllocator, sampler_spec);
		}
		// Initializing linear filtration sampler
		{
			ImageSamplerSpecification sampler_spec = {};
			sampler_spec.min_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.mag_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.mipmap_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.address_mode = SamplerAddressMode::REPEAT;
			sampler_spec.min_lod = 0.0f;
			sampler_spec.max_lod = 1000.0f;
			sampler_spec.lod_bias = 0.0f;
			sampler_spec.anisotropic_filtering_level = m_Specification.anisotropic_filtering;

			m_SamplerLinear = ImageSampler::Create(&g_PersistentAllocator, sampler_spec);
		}
		// Load dummy white texture
		{
			RGBA32 image_data(255, 255, 255, 255);

			ImageSpecification image_spec = ImageSpecification::Default();
			image_spec.usage = ImageUsage::TEXTURE;
			image_spec.extent = { 1, 1, 1 };
			image_spec.pixels = std::vector<byte>((byte*)&image_data, (byte*)(&image_data + 1));
			image_spec.format = ImageFormat::RGBA32_UNORM;
			image_spec.type = ImageType::TYPE_2D;
			image_spec.mip_levels = 1;
			image_spec.array_layers = 1;
			OMNI_DEBUG_ONLY_CODE(image_spec.debug_name = "Dummy white texture");

			m_DummyWhiteTexture = Image::Create(&g_PersistentAllocator, image_spec, 0);
			AcquireResourceIndex(m_DummyWhiteTexture, SamplerFilteringMode::LINEAR);
		}
		// Create render queue
		{
			DeviceBufferSpecification spec = {};

			// Create initial render queue
			spec.size = sizeof(InstanceRenderData) * std::pow(2, 16) * Renderer::GetConfig().frames_in_flight;
			spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
			spec.heap = DeviceBufferMemoryHeap::DEVICE;
			spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
			OMNI_DEBUG_ONLY_CODE(spec.debug_name = "Device render queue");

			m_DeviceRenderQueue = DeviceBuffer::Create(&g_PersistentAllocator, spec);
		}
	}

	ISceneRenderer::~ISceneRenderer()
	{

	}

	uint32 ISceneRenderer::AcquireResourceIndex(Ref<Image> image, SamplerFilteringMode filtering_mode)
	{
		std::lock_guard lock(m_Mutex);
		uint32 index = m_TextureIndexAllocator->Allocate(4, 1) / sizeof uint32;

		Ref<ImageSampler> sampler;

		switch (filtering_mode)
		{
		case SamplerFilteringMode::NEAREST:		sampler = m_SamplerNearest; break;
		case SamplerFilteringMode::LINEAR:		sampler = m_SamplerLinear;  break;
		default:								OMNIFORCE_ASSERT_TAGGED(false, "Invalid sampler filtering mode"); break;
		}

		for (auto& set : m_SceneDescriptorSet)
			set->Write(0, index, image, sampler);
		m_TextureIndices.emplace(image->Handle, index);

		return index;
	}

	uint32 ISceneRenderer::AcquireResourceIndex(Ref<Material> material)
	{
		return m_MaterialDataPool.Allocate(material->Handle);
	}

	uint32 ISceneRenderer::AcquireResourceIndex(Ref<Mesh> mesh)
	{
		std::lock_guard lock(m_Mutex);

		GeometryMeshData mesh_data = {};
		mesh_data.lod_distance_multiplier = 1.0f;
		mesh_data.bounding_sphere = mesh->GetBoundingSphere();
		mesh_data.meshlet_count = mesh->GetMeshletCount();
		mesh_data.quantization_grid_size = mesh->GetQuantizationGridSize();
		mesh_data.ray_tracing.layout = mesh->GetAttributeLayout();
		mesh_data.vertices = mesh->GetBuffer(MeshBufferKey::GEOMETRY)->GetDeviceAddress();
		mesh_data.attributes = mesh->GetBuffer(MeshBufferKey::ATTRIBUTES)->GetDeviceAddress();
		mesh_data.meshlets_data = mesh->GetBuffer(MeshBufferKey::MESHLETS)->GetDeviceAddress();
		mesh_data.micro_indices = mesh->GetBuffer(MeshBufferKey::MICRO_INDICES)->GetDeviceAddress();
		mesh_data.meshlets_cull_bounds = mesh->GetBuffer(MeshBufferKey::MESHLETS_CULL_DATA)->GetDeviceAddress();
		mesh_data.ray_tracing.indices = mesh->GetBuffer(MeshBufferKey::RT_INDICES)->GetDeviceAddress();
		mesh_data.ray_tracing.attributes = mesh->GetBuffer(MeshBufferKey::RT_ATTRIBUTES)->GetDeviceAddress();

		return m_MeshResourcesBuffer.Allocate(mesh->Handle, mesh_data);
	}

	uint32 ISceneRenderer::AcquireResourceIndex(Ref<Image> image)
	{
		OMNIFORCE_ASSERT_TAGGED(false, "Not implemented");
		std::unreachable();
	}

}