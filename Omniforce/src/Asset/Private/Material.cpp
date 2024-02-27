#include "../Material.h"

#include <Log/Logger.h>
#include <Foundation/Macros.h>
#include <Memory/Pointers.hpp>
#include <Renderer/ShaderLibrary.h>
#include <Scene/PipelineLibrary.h>

namespace Omni {

	Shared<Material> Material::Create(std::string name, AssetHandle id /*= AssetHandle()*/)
	{
		return std::make_shared<Material>(name, id);
	}

	uint8 Material::GetRuntimeEntrySize(uint8 variant_index)
	{
		switch (variant_index)
		{
		case 0: return 4;
		case 1: return 4;
		case 2: return 4;
		case 3: return 16;
		default: OMNIFORCE_ASSERT_TAGGED(false, "Invalid material entry type index");
		}

		return 0;
	}

	void Material::Destroy()
	{
		
	}

	void Material::CompilePipeline()
	{
		ShaderLibrary* shader_library = ShaderLibrary::Get();

		Shared<Shader> shader = shader_library->GetShader("uber_main.ofs", m_Macros);

		if (!shader) {
			shader_library->LoadShader("Resources/shaders/uber_main.ofs", m_Macros);
			shader = shader_library->GetShader("uber_main.ofs", m_Macros);
		}

		PipelineSpecification pipeline_spec = PipelineSpecification::Default();
		pipeline_spec.shader = shader;
		pipeline_spec.output_attachments_formats = { ImageFormat::RGBA64_SFLOAT, ImageFormat::RGBA32_UNORM, ImageFormat::RGBA64_SFLOAT, ImageFormat::RGBA32_UNORM };
		pipeline_spec.culling_mode = PipelineCullingMode::NONE;
		pipeline_spec.depth_test_enable = true;

		PipelineLibrary::HasPipeline(pipeline_spec) ? 
			m_Pipeline = PipelineLibrary::GetPipeline(pipeline_spec) : m_Pipeline = Pipeline::Create(pipeline_spec);

		m_Macros.clear();
	}

}
