#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"

#include <atomic>
#include <unordered_map>

#include <robin_hood.h>

namespace Omni {

	class OMNIFORCE_API UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID& other);

		uint64 Get() const { return m_UUID; }
		bool Valid() const { return m_UUID != 0; }

		operator uint64() const { return m_UUID; }

	private:
		uint64_t m_UUID;
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