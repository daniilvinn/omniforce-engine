#pragma once

#include <Foundation/Types.h>

#include <string>
#include <ctime>
#include <iostream>

#include <fmt/format.h>
#include <glm/gtx/matrix_decompose.hpp>

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
			
			if (tt->tm_mon < 9)
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

		inline void DecomposeMatrix(const glm::mat4& source, glm::vec3* translation, glm::vec3* rotation, glm::vec3* scale) {
			glm::quat orientation;
			glm::vec3 skew;
			glm::vec4 perpective;

			glm::decompose(source, *scale, orientation, *translation, skew, perpective);
			*rotation = glm::degrees(glm::eulerAngles(orientation));
		};

	}
}