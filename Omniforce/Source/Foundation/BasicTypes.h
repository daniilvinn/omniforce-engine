#pragma once

#include <Foundation/UUID.h>

#include <filesystem>
#include <cstdint>
#include <map>
#include <limits>
#include <atomic>

#include <glm/glm.hpp>

namespace glm {

	using hvec2 = glm::u16vec2;
	using hvec3 = glm::u16vec3;
	using hvec4 = glm::u16vec4;

}

namespace Omni {

	typedef uint64_t uint64;
	typedef uint32_t uint32;
	typedef uint16_t uint16;
	typedef uint8_t  uint8;

	typedef int64_t int64;
	typedef int32_t int32;
	typedef int16_t int16;
	typedef int8_t  int8;

	typedef uint8_t byte;
	typedef float float32;
	typedef double float64;
	typedef glm::uint16_t half;

	using BitMask = uint64;
	using Flag = uint64;
	using AssetHandle = UUID;

	template <typename T>
	using Atomic = std::atomic<T>;

	namespace rh = robin_hood;
	namespace stdfs = std::filesystem;

	template<typename Key, typename Value>
	using rhumap = rh::unordered_map<Key, Value>;

	inline constexpr float32 FP32_MAX = std::numeric_limits<float32>::max();
	inline constexpr float64 FP64_MAX = std::numeric_limits<float64>::max();

	inline constexpr float32 FP32_MIN = std::numeric_limits<float32>::min();
	inline constexpr float64 FP64_MIN = std::numeric_limits<float64>::min();

	// Deprecated
	using ivec2 = glm::ivec2;
	using ivec3 = glm::ivec3;
	using ivec4 = glm::ivec4;

	using uvec2 = glm::uvec2;
	using uvec3 = glm::uvec3;
	using uvec4 = glm::uvec4;

	using fvec2 = glm::vec2;
	using fvec3 = glm::vec3;
	using fvec4 = glm::vec4;
	// End deprecated

	struct MiscData {
		byte* data;
		uint32 size;
	};

	using RGB32 = glm::u8vec3;
	using RGBA32 = glm::u8vec4;

	struct META(ShaderExpose, Module = "Common") Sphere {
		glm::vec3 center = {};
		float32 radius = {};
	};

	struct META(ShaderExpose, Module = "Common") AABB {
		glm::vec3 min = {}, max = {};
	};

	struct META(ShaderExpose, Module = "Common") AABB_2D {
		glm::vec2 min = {}, max = {};
	};

	struct META(ShaderExpose, Module = "Common") Bounds {
		Sphere sphere = {};
		AABB aabb = {};
	};

	struct META(ShaderExpose, Module = "Common") Plane {
		glm::vec3 normal = {};
		float32 distance = {};

		Plane() {
			normal = { 0.0f, 1.0f, 0.0f };
			distance = 0.0f;
		}

		Plane(const glm::vec3& p1, const glm::vec3& norm)
			: normal(glm::normalize(norm)),
			distance(glm::dot(normal, p1))
		{}

	};

	struct Frustum {
		Plane planes[6] = {}; // top, bottom, right, left, far, near planes
	};

	// Macro table is usually being iterated, so I use an array of pairs and not a map, because map
	// fits better for random access
	using ShaderMacroTable = std::map<std::string, std::string>;

}