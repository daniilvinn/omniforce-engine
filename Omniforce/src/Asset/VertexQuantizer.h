#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>

namespace Omni {

	class OMNIFORCE_API VertexDataQuantizer {
	public:
		// NOTE: mesh bitrate doesn't equal to vertex bitrate.
		// Vertex bitrate is precision within a single unit (in our case - a single meter, so 1.0f)
		// and is used to determine how much precision there can be within a single unit. 
		// Mesh bitrate is final bitrate to encode vertices with, which is computed 
		// as vertex bitrate + num of bits needed to encode num of units used in a mesh.
		// So if there's 4 meters (units) long mesh, we would 
		// need `vertex_bitrate + 3 (num of bits to encode number 4)` bits.
		uint32 ComputeMeshBitrate(uint32 vertex_bitrate, const AABB& aabb) {
			uint32 result = 0;
			for (int i = 0; i < 3; i++) {
				float32 max_dimension_length = aabb.max[i] - aabb.min[i];
				uint32 desired_bitrate = std::ceil(std::log2(max_dimension_length * (1u << vertex_bitrate)));
				if (desired_bitrate > result)
					result = desired_bitrate;
			}
			return result;
		}

		// Quantize by addition of AABB's channel min value to remove sign and multiplying by unit grid size (`1u << bitrate`)
		uint32 QuantizeVertexChannel(float32 f, uint32 local_bitrate, float32 aabb_channel_min) {
			return (f + std::abs(aabb_channel_min)) * (1u << local_bitrate);
		}

		// Decode it by subtracting encoded value's exponent with vertex bitrate 
		// (ldexp() with negative argument), which is equal to division by POT value (`1 << local_bitrate`)
		// and subtract AABB's min value to restore sign
		float32 DequantizeVertexChannel(uint32 f, uint32 local_bitrate, float32 aabb_channel_min) {
			return std::ldexpf((float)f, -local_bitrate) - std::abs(aabb_channel_min);
		}

		glm::u16vec2 QuantizeNormal(glm::vec3 n) {
			n /= (glm::abs(n.x) + glm::abs(n.y) + glm::abs(n.z));
			n = glm::vec3(n.z > 0.0f ? glm::vec2(n.x, n.y) : OctWrap(glm::vec2(n.x, n.y)), n.z);
			n = glm::vec3(glm::vec2(n.x, n.y) * 0.5f + 0.5f, n.z);

			return glm::packHalf(glm::vec2(n.x, n.y));
		}

		glm::vec3 DequantizeNormal(glm::u16vec2 v)
		{
			glm::vec2 f = glm::unpackHalf(v);

			f = f * 2.0f - 1.0f;

			// https://twitter.com/Stubbesaurus/status/937994790553227264
			glm::vec3 n = glm::vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
			float t = glm::max(-n.z, 0.0f);
			n.x += n.x >= 0.0f ? -t : t;
			n.y += n.y >= 0.0f ? -t : t;

			return glm::normalize(n);
		}

		glm::u16vec2 QuantizeTangent(glm::vec4 t) {
			glm::u16vec2 q = QuantizeNormal(glm::vec3(t.x, t.y, t.z));

			// If bitangent sign is positive, we set first bit of Y component to 1, otherwise (if it is negative) we set 0
			q.y = t.w == 1.0f ? q.y | 1u : q.y & ~1u;

			return q;
		}

		glm::vec4 DequantizeTangent(glm::u16vec2 t) {
			glm::vec3 n = DequantizeNormal(t);

			// Decode bitangent sign
			return glm::vec4(n, t.y & 1 ? 1.0 : -1.0f);
		}

		glm::u16vec2 QuantizeUV(glm::vec2 uv) {
			return glm::packHalf(uv);
		}

		static uint32 GetRuntimeAttributeSize(std::string_view key) {
			// if key is NORMAL, TANGENT, TEXCOORD_n, COLOR_n, return 4 bytes size (f16vec2 for all attributes except color, which is u8vec4)
			if (key == "NORMAL" || key == "TANGENT" || key.find("TEXCOORD") != std::string::npos || key.find("COLOR") != std::string::npos)
				return 4;
			else if (key.find("JOINTS") != std::string::npos || key.find("WEIGHTS") != std::string::npos)
				return 8;
			else 
				OMNIFORCE_ASSERT_TAGGED(false, "Unknown attribute key");

			return 0;
		}

	private:
		glm::vec2 OctWrap(glm::vec2 v) {
			glm::vec2 w = 1.0f - glm::abs(glm::vec2(v.y, v.x));
			if (v.x < 0.0f) w.x = -w.x;
			if (v.y < 0.0f) w.y = -w.y;
			return w;
		}

	};

}