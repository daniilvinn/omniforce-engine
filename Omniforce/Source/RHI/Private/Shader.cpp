#include <Foundation/Common.h>
#include <RHI/Shader.h>

#include <Platform/Vulkan/VulkanShader.h>

namespace Omni {

	Ref<Shader> Shader::Create(IAllocator* allocator, std::map<ShaderStage, std::vector<uint32>> binaries, std::filesystem::path path)
	{
		return CreateRef<VulkanShader>(allocator, binaries, path);
	}

}