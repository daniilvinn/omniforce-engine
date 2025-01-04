#include <Foundation/Common.h>
#include <Renderer/Shader.h>

#include <Platform/Vulkan/VulkanShader.h>

namespace Omni {

	Shared<Shader> Shader::Create(std::map<ShaderStage, std::vector<uint32>> binaries, std::filesystem::path path)
	{
		return std::make_shared<VulkanShader>(binaries, path);
	}

}