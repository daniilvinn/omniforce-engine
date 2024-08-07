#include "../VulkanDescriptorSet.h"

#include "../VulkanGraphicsContext.h"
#include "../VulkanRenderer.h"

#include "../VulkanImage.h"
#include "../VulkanDeviceBuffer.h"

namespace Omni {

#pragma region converts
	constexpr VkDescriptorType UsageToDescriptorType(const DeviceBufferUsage& usage) {
		switch (usage)
		{
		case DeviceBufferUsage::UNIFORM_BUFFER:		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case DeviceBufferUsage::STORAGE_BUFFER:		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		default:									std::unreachable();
		}
	}

	constexpr VkDescriptorType convert(const DescriptorBindingType& type) {
		switch (type)
		{
		case DescriptorBindingType::SAMPLED_IMAGE:		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case DescriptorBindingType::STORAGE_IMAGE:		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case DescriptorBindingType::UNIFORM_BUFFER:		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case DescriptorBindingType::STORAGE_BUFFER:		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		default:										std::unreachable();
		}
	}
#pragma endregion

	constexpr VkDescriptorBindingFlags extractFlags(const BitMask& mask) {
		VkDescriptorBindingFlags flags = 0;

		if (mask & (uint32)DescriptorFlags::PARTIALLY_BOUND) flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

		return flags;
	}

	VulkanDescriptorSet::VulkanDescriptorSet(const DescriptorSetSpecification& spec)
		: m_DescriptorSet(VK_NULL_HANDLE), m_Layout(VK_NULL_HANDLE)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorBindingFlags> binding_flags;

		for (auto& binding : spec.bindings) {
			VkDescriptorSetLayoutBinding vk_binding = {};
			vk_binding.stageFlags = VK_SHADER_STAGE_ALL;
			vk_binding.pImmutableSamplers = nullptr;
			vk_binding.descriptorCount = binding.array_count;
			vk_binding.descriptorType = convert(binding.type);
			vk_binding.binding = binding.binding;

			bindings.push_back(vk_binding);
			binding_flags.push_back(extractFlags(binding.flags));
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo vk_binding_flags = {};
		vk_binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		vk_binding_flags.bindingCount = spec.bindings.size();
		vk_binding_flags.pBindingFlags = binding_flags.data();

		VkDescriptorSetLayoutCreateInfo layout_create_info = {};
		layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_create_info.pNext = &vk_binding_flags;
		layout_create_info.bindingCount = bindings.size();
		layout_create_info.pBindings = bindings.data();
		layout_create_info.flags = 0;

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->Raw(), &layout_create_info, nullptr, &m_Layout));

		std::vector<VkDescriptorSet> set = VulkanRenderer::AllocateDescriptorSets(m_Layout, 1);
		m_DescriptorSet = set[0];

	}

	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();
		vkDestroyDescriptorSetLayout(device->Raw(), m_Layout, nullptr);
		VulkanRenderer::FreeDescriptorSets({ m_DescriptorSet });
	}

	void VulkanDescriptorSet::Destroy()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();
		VulkanRenderer::FreeDescriptorSets({ m_DescriptorSet });
		vkDestroyDescriptorSetLayout(device->Raw(), m_Layout, nullptr);
	}

	void VulkanDescriptorSet::Write(uint16 binding, uint16 array_element, Shared<Image> image, Shared<ImageSampler> sampler)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();
		Shared<VulkanImage> vk_image = ShareAs<VulkanImage>(image);
		Shared<VulkanImageSampler> vk_sampler = ShareAs<VulkanImageSampler>(sampler);

		VkDescriptorImageInfo descriptor_image_info = {};
		descriptor_image_info.imageView = vk_image->RawView();
		descriptor_image_info.sampler = vk_sampler->Raw();
		descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write_descriptor_set = {};
		write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set.dstSet = m_DescriptorSet;
		write_descriptor_set.dstBinding = binding;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_descriptor_set.dstArrayElement = array_element;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.pImageInfo = &descriptor_image_info;

		vkUpdateDescriptorSets(device->Raw(), 1, &write_descriptor_set, 0, nullptr);
	}

	void VulkanDescriptorSet::Write(uint16 binding, uint16 array_element, Shared<DeviceBuffer> buffer, uint64 size, uint64 offset)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();
		Shared<VulkanDeviceBuffer> vk_buffer = ShareAs<VulkanDeviceBuffer>(buffer);

		VkDescriptorBufferInfo descriptor_buffer_info = {};
		descriptor_buffer_info.buffer = vk_buffer->Raw();
		descriptor_buffer_info.range = size;
		descriptor_buffer_info.offset = offset;

		VkWriteDescriptorSet write_descriptor_set = {};
		write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set.dstSet = m_DescriptorSet;
		write_descriptor_set.dstBinding = binding;
		write_descriptor_set.descriptorType = UsageToDescriptorType(vk_buffer->GetSpecification().buffer_usage);
		write_descriptor_set.dstArrayElement = array_element;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.pBufferInfo = &descriptor_buffer_info;

		vkUpdateDescriptorSets(device->Raw(), 1, &write_descriptor_set, 0, nullptr);
	}

}