#pragma once

#include "Core/UUID.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <filesystem>
#include <Memory/Pointers.hpp>

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

	using BitMask = uint64;
	using Flag = uint64;
	using AssetHandle = UUID;

	namespace rh = robin_hood;
	namespace stdfs = std::filesystem;

	template<typename T = float32>
	struct vec2 {
		union {
			T values[2];
			struct {
				union {
					T x, r;
				};
				union {
					T y, g;
				};
			};
		};

		template<typename T>
		explicit constexpr operator vec2<T>()   const	{ return { (T)x, (T)y }; }
		vec2 operator+(const vec2& other) { return { x + other.x, y + other.y }; }
		vec2 operator-(const vec2& other) { return { x - other.x, y - other.y }; }
		vec2& operator+=(const vec2& other) { this = this + other; return *this; }
		vec2& operator-=(const vec2& other) { this = this - other; return *this; }

		operator glm::vec<2, T, glm::defaultp>() {
			return glm::vec<2, T, glm::defaultp>(x, y);
		}
	};

	template<typename T = float32>
	struct vec3 {
		union {
			T x, r;
		};
		union {
			T y, g;
		};
		union {
			T z, b;
		};

		template<typename T>
		explicit constexpr operator vec3<T>() const	{ return { T(x), T(y), T(z)}; }
		vec3 operator+(const vec3& other) { return { x + other.x, y + other.y, z + other.z }; }
		vec3 operator-(const vec3& other) { return { x - other.x, y - other.y, z - other.z }; }
		vec3& operator+=(const vec3& other) { this = this + other; return *this; }
		vec3& operator-=(const vec3& other) { this = this - other; return *this; }

		operator glm::vec<3, T, glm::defaultp>() {
			return glm::vec<3, T, glm::defaultp>(x, y, z);
		}
	};

	template<typename T = float32>
	struct vec4 {
		union {
			T x, r;
		};
		union {
			T y, g;
		};
		union {
			T z, b;
		};
		union {
			T w, a;
		};

		template<typename T>
		explicit constexpr operator vec4<T>()   const { return { (T)x, (T)y, T(z), (T)w }; }

		vec4 operator+(const vec4& other) { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
		vec4 operator-(const vec4& other) { return { x - other.x, y - other.y, z - other.z, w - other.w }; }
		vec4& operator+=(const vec4& other) { this = this + other; return *this; }
		vec4& operator-=(const vec4& other) { this = this - other; return *this; }

		operator glm::vec<4, T, glm::defaultp>() {
			return glm::vec<4, T, glm::defaultp>(x, y, z, w);
		}
	};

	using ivec2 = vec2<int32>;
	using ivec3 = vec3<int32>;
	using ivec4 = vec4<int32>;

	using uvec2 = vec2<uint32>;
	using uvec3 = vec3<uint32>;
	using uvec4 = vec4<uint32>;

	using fvec2 = vec2<float32>;
	using fvec3 = vec3<float32>;
	using fvec4 = vec4<float32>;

	struct MiscData {
		byte* data;
		uint32 size;
	};

	using RGB32 = glm::u8vec3;
	using RGBA32 = glm::u8vec4;

	struct Sphere {
		glm::vec3 center;
		float32 radius;
	};

	struct AABB {
		glm::vec3 min, max;
	};

}