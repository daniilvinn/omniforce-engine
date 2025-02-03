#pragma once

#if __OMNI_COMPILE_HOST_CODE
	#include <Shaders/Slang/Shared/TypeAlises.h>
	using glm::dot;
	#define SHARED_MODULE_EXPORT
#else
	module CoreTypes;
	#define SHARED_MODULE_EXPORT public
#endif

namespace Omni {

	SHARED_MODULE_EXPORT struct Sphere {
		float3 center;
		float radius;
	};

    SHARED_MODULE_EXPORT struct AABB {
		float3 min;
		float3 max;
	};

    SHARED_MODULE_EXPORT struct AABB_2D {
		float2 min;
		float2 max;
	};

    SHARED_MODULE_EXPORT struct Bounds {
		Sphere sphere = {};
		AABB aabb = {};
	};

    SHARED_MODULE_EXPORT struct Plane {
		float3 normal;
		float distance;
	};

    SHARED_MODULE_EXPORT struct Frustum {
		Plane planes[6];

        SHARED_MODULE_EXPORT bool TestBoundingSphere(Sphere sphere) {
			bool result = true;

			for (int i = 0; i < 6; i++) {
				Plane plane = planes[i];
				result = result && ((dot(plane.normal, sphere.center) - plane.distance) > -sphere.radius);
			}
			return result;
		}
	}

}

#if !__OMNI_COMPILE_HOST_CODE
#undef SHARED_MODULE_EXPORT
#endif