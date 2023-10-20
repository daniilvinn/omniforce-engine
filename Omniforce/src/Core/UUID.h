#pragma once

#include "Foundation/Macros.h"

#include <atomic>
#include <unordered_map>

#include <robin_hood.h>

namespace Omni {

	typedef uint64_t uint64;

	class OMNIFORCE_API UUID
	{
	public:
		UUID();
		UUID(uint64 uuid);
		UUID(const UUID& other);

		uint64 Get() const { return m_UUID; }
		bool Valid() const { return m_UUID != 0; }

		operator uint64() const { return m_UUID; }

	private:
		uint64 m_UUID;
	};

}

namespace robin_hood {

	template<>
	struct hash<Omni::UUID> {
		size_t operator()(const Omni::UUID& uuid) const {
			return hash<Omni::uint64>()(uuid.Get());
		}
	};

}