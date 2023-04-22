#pragma once
#include <cstdint>
#include <glm/glm.hpp>

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

	template<typename T = float32>
	struct alignas(8) vec2 {
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

		explicit constexpr operator vec2<int32>()   const	{ return { (int32)x, (int32)y }; }
		explicit constexpr operator vec2<uint32>()  const	{ return { (uint32)x, (uint32)y }; }
		explicit constexpr operator vec2<float32>() const { return { (float32)x, (float32)y }; }

		T operator[](uint32 index) { return values[index]; }
	};

	template<typename T = float32>
	struct alignas(16) vec3 {
		union {
			T values[3];
			struct {
				union {
					T x, r;
				};
				union {
					T y, g;
				};
				union {
					T z, b;
				};
			};
		};

		explicit constexpr operator vec3<int32>()   const	{ return { (int32)values }; }
		explicit constexpr operator vec3<uint32>()  const	{ return { (uint32)values }; }
		explicit constexpr operator vec3<float32>() const { return { (float32)values }; }

		T operator[](uint32 index) { return values[index]; }
	};

	template<typename T = float32>
	struct alignas(16) vec4 {
		union {
			T values[4];
			struct {
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
			};
		};

		explicit constexpr operator vec4<int32>()   const { return { (int32)values }; }
		explicit constexpr operator vec4<uint32>()  const { return { (uint32)values }; }
		explicit constexpr operator vec4<float32>() const { return { (float32)values }; }

		T operator[](uint32 index) { return values[index]; }
	};

	template<typename T>
	struct alignas(64) mat4 {
		vec4<T> values[4];
		vec4<T> operator[](uint32 index) { return values[index]; }
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

	using imat4 = mat4<int32>;
	using umat4 = mat4<uint32>;
	using fmat4 = mat4<float32>;

	struct MiscData {
		void* data;
		uint32 size;
	};

}