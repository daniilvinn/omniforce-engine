#pragma once

#include <Foundation/Platform.h>

#include <random>
#include <limits>

namespace Omni {

	class OMNIFORCE_API RandomEngine {
	public:
		template<typename T>
		inline static T Generate(const T& min = std::numeric_limits<T>::min(), const T& max = std::numeric_limits<T>::max()) {
			static_assert(std::is_integral<T>()); // check for non-integral type
			T value = (s_UniformDistribution(s_MersenneTwisterGenerator) % (max - min)) + min;
			return value;
		}
	private:
		inline static thread_local std::random_device s_RandomDevice;
		inline static thread_local std::mt19937_64 s_MersenneTwisterGenerator = std::mt19937_64(s_RandomDevice());
		inline static thread_local std::uniform_int_distribution<uint64> s_UniformDistribution;
	};

}