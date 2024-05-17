#pragma once

#include <Foundation/Types.h>
#include <Foundation/Macros.h>

#include <random>
#include <limits>

namespace Omni {

	class OMNIFORCE_API RandomEngine {
	public:
		template<typename T>
		inline static T Generate(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
			OMNIFORCE_ASSERT_TAGGED(std::is_integral<T>, "Non-integral template parameter");
			T value = (s_UniformDistribution(s_MersenneTwisterGenerator) % (max - min)) + min;
			return value;
		}
	private:
		inline static thread_local std::random_device s_RandomDevice;
		inline static thread_local std::mt19937_64 s_MersenneTwisterGenerator = std::mt19937_64(s_RandomDevice());
		inline static thread_local std::uniform_int_distribution<uint64> s_UniformDistribution;
	};

}