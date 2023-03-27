#include "../Shader.h"

#include <Platform/Vulkan/VulkanShader.h>

namespace Omni {

	Shared<Shader> Shader::Create(const ShaderStage& stage, const std::vector<uint32>& code, std::filesystem::path path)
	{
		return std::make_shared<VulkanShader>(stage, code, path);
	}

	ShaderProgram::ShaderProgram(std::initializer_list<Shared<Shader>> shaders)
		: m_Shaders(shaders) {}

}