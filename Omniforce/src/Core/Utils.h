#pragma once

#include <Foundation/Types.h>

#include <string>
#include <ctime>
#include <iostream>
#include <type_traits>
#include <vector>

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <omp.h>

namespace Omni {

	namespace Utils {

		inline const std::string EvaluateDatetime() 
		{
			std::string datetime;

			time_t t;
			struct tm* tt;
			time(&t);
			tt = localtime(&t);

			datetime += std::to_string(tt->tm_year + 1900);
			
			if (tt->tm_mon < 10)
				datetime += "0";

			datetime += std::to_string(tt->tm_mon + 1);

			if (tt->tm_mday < 10)
				datetime += "0";

			datetime += std::to_string(tt->tm_mday);

			if(tt->tm_hour < 10)
				datetime += "0";

			datetime += std::to_string(tt->tm_hour);

			if (tt->tm_min < 10)
				datetime += "0";

			datetime += std::to_string(tt->tm_min);

			return datetime;
		}

		// Thanks to https://github.com/SaschaWillems for this alignment function
		inline uint64 Align(uint64 original, uint64 min_alignment) {
			size_t alignedSize = original;
			if (min_alignment > 0) {
				alignedSize = (alignedSize + min_alignment - 1) & ~(min_alignment - 1);
			}
			return alignedSize;
		}

		inline constexpr std::string FormatAllocationSize(uint64 size) {
			if (size / 1'000'000'000 >= 1)
				return fmt::format("{:.4}(GiB)", (double)size / 1'000'000'000.0);
			else if (size / 1'000'000 >= 1)
				return fmt::format("{:.4}(MiB)", (double)size / 1'000'000.0);
			else if (size / 1'000 >= 1)
				return fmt::format("{:.4}(KiB)", (double)size / 1'000.0);
			else
				return fmt::format("{}(B)", size);
		}

		inline void DecomposeMatrix(const glm::mat4& source, glm::vec3* translation, glm::quat* rotation, glm::vec3* scale) {
			glm::vec3 skew;
			glm::vec4 perpective;

			glm::decompose(source, *scale, *rotation, *translation, skew, perpective);
		};

		inline glm::mat4 ComposeMatrix(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
		{
			return glm::translate(glm::mat4(1.0f), translation) *
				glm::mat4_cast(rotation) *
				glm::scale(glm::mat4(1.0f), scale);

		}
	
		template<typename T>
		inline bool IsPowerOf2(T value) {
			if (std::is_integral_v<T>)
				return false;

			return value && (value & (value - 1) == 0);
		}

		// NOTE: returns amount of mip levels excluding mip #0
		inline uint8 ComputeNumMipLevelsBC7(uint16 image_width, uint16 image_height) {
			return log2(std::min(image_width, image_height)) - 2;
		}

		// including mip0 (source image)
		template<uint32 PixelSize>
		inline uint32 ComputeMipLevelsStorage(uint16 image_width, uint16 image_height) {
			uint32 mip_levels_req = log2(std::min(image_width, image_height)) - 2;

			uint32 current_width = image_width;
			uint32 current_height = image_height;

			uint32 storage_req = current_width * current_height * PixelSize;

			for (int32 i = 1; i < mip_levels_req + 1; i++) {
				uint32 req_storage_for_current_mip = (current_width / 2) * (current_height / 2);
				storage_req += req_storage_for_current_mip * PixelSize;
				current_width /= 2;
				current_height /= 2;
			}

			return storage_req;
		}

	}
}