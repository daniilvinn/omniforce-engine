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

	template <typename T = int>
	struct Rect2D 
	{
		T x, y;
	};

	using WindowSize2D = Rect2D<uint16>;
	using BitMask = uint64;

	template<typename T = float32>
	struct vec2 {
		union {
			T x, r;
		};
		union {
			T y, g;
		};
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
	};

}