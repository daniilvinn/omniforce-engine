#pragma once

#include <Filesystem/Filesystem.h>

#include <shaderc/shaderc.hpp>
#include <array>

namespace Omni {

	class ShaderSourceIncluder : public shaderc::CompileOptions::IncluderInterface {
	public:
		shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override
		{
			auto* includeResult = new shaderc_include_result();

			auto* container = new std::array<std::string, 2>();
			includeResult->user_data = container;

			auto file = FileSystem::ReadFile(std::filesystem::path("Resources/shaders") / requested_source, (BitMask)FileReadingFlags::READ_BINARY);

			auto shaderSrc = file->GetData();
			
			(*container)[0] = requested_source;
			(*container)[1] = std::move(std::string(shaderSrc, shaderSrc + file->GetSize()));

			includeResult->source_name = (*container)[0].data();
			includeResult->source_name_length = (*container)[0].size();
			includeResult->content = (*container)[1].data();
			includeResult->content_length = (*container)[1].size();

			file->Release();

			return includeResult;
		}

		void ReleaseInclude(shaderc_include_result* data) override
		{
			delete static_cast<std::array<std::string, 2>*>(data->user_data);
			delete data;
		}

	};

}