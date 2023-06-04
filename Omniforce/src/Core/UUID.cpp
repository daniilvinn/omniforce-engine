#include "UUID.h"

#include <random>

#include <random>

namespace Omni {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 eng(s_RandomDevice());
	static std::uniform_int_distribution<uint64> s_UniformDistribution;

	UUID::UUID()
	{
		m_UUID = s_UniformDistribution(eng);
	}

	UUID::UUID(uint64_t uuid)
		: m_UUID(uuid)
	{

	}

	UUID::UUID(const UUID& other)
		: m_UUID(other.m_UUID)
	{

	}

}