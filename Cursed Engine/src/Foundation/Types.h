#pragma once
#include <cstdint>

namespace Cursed {

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

	template<typename T = float32>
	struct vec2 {
		union {
			T x, r;
		};
		union {
			T y, g;
		};

		explicit operator vec2<int32>() const	{ return { (int32)x, (int32)y }; }
		explicit operator vec2<uint32>() const	{ return { (uint32)x, (uint32)y }; }
		explicit operator vec2<float32>() const { return { (float32)x, (float32)y }; }
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

		explicit operator vec3<int32>() const	{ return { (int32)x, (int32)y, (int32)z }; }
		explicit operator vec3<uint32>() const	{ return { (uint32)x, (uint32)y, (uint32) z }; }
		explicit operator vec3<float32>() const { return { (float32)x, (float32)y, (float32) z }; }
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

		explicit operator vec4<int32>() const	{ return { (int32)x, (int32)y, (int32)z, (int32)w }; }
		explicit operator vec4<uint32>() const	{ return { (uint32)x, (uint32)y, (uint32) z, (uint32)w }; }
		explicit operator vec4<float32>() const { return { (float32)x, (float32)y, (float32) z, (float32)w }; }
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

}