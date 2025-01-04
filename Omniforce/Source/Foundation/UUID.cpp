#include <Foundation/UUID.h>

#include <Foundation/RandomNumberGenerator.h>

namespace Omni {

	UUID::UUID()
	{
		m_UUID = RandomEngine::Generate<uint64>();
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