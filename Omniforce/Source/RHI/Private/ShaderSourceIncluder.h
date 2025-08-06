#pragma once

#include <Foundation/Common.h>
#include <Filesystem/Filesystem.h>

#include <array>

#include <shaderc/shaderc.hpp>

namespace Omni {

	class ShaderSourceIncluder : public shaderc::CompileOptions::IncluderInterface {
	public:
		ShaderSourceIncluder(const std::vector<std::string> SystemIncludePaths)
			: mIncludePaths(SystemIncludePaths) {}

		shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override
		{
			auto* include_result = new shaderc_include_result();

			std::string resolved_path;
			if (!ResolveIncludePath(requested_source, resolved_path)) {
				include_result->source_name = requested_source;
				include_result->source_name_length = strlen(requested_source);
				include_result->content = nullptr;
				include_result->content_length = 0;
				include_result->user_data = nullptr;

				OMNIFORCE_CORE_ERROR("Preprocessing shader {}: failed to find {} source file", requested_source, requested_source);

				return include_result;
			}

			auto* container = new std::array<std::string, 2>();
			include_result->user_data = container;

			auto file = FileSystem::ReadFile(&g_TransientAllocator, resolved_path, (BitMask)FileReadingFlags::READ_BINARY);

			auto shaderSrc = file->GetData();
			
			(*container)[0] = requested_source;
			(*container)[1] = std::move(std::string(shaderSrc, shaderSrc + file->GetSize()));

			include_result->source_name = (*container)[0].data();
			include_result->source_name_length = (*container)[0].size();
			include_result->content = (*container)[1].data();
			include_result->content_length = (*container)[1].size();

			file->Release();

			return include_result;
		}

		void ReleaseInclude(shaderc_include_result* data) override
		{
			delete static_cast<std::array<std::string, 2>*>(data->user_data);
			delete data;
		}

	private:
		bool ResolveIncludePath(const std::string& requested_source, std::string& resolved_path) {
			for (const auto& path : mIncludePaths) {
				std::string full_path = path + "/" + requested_source;
				if (FileSystem::CheckDirectory(full_path)) {
					resolved_path = full_path;
					return true;
				}
			}
			return false; // Not found
		}

	private:

		std::vector<std::string> mIncludePaths;

	};

}