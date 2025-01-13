#pragma once

#include <Foundation/Platform.h>

#include <Foundation/RandomNumberGenerator.h>

#include <atomic>
#include <unordered_map>

#include <spdlog/fmt/fmt.h>
#include <robin_hood.h>

namespace Omni {

	typedef uint64_t uint64;

	class OMNIFORCE_API UUID
	{
	public:
		UUID()
		{
			m_UUID = RandomEngine::Generate<uint64>();
		}

		UUID(uint64 uuid)
			: m_UUID(uuid)
		{}
		
		UUID(const UUID& other)
			: m_UUID(other.m_UUID)
		{}

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

template<>
struct fmt::formatter<Omni::UUID>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& ctx) { return ctx.begin(); };

	template<typename FormatContext>
	auto format(Omni::UUID const& number, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "{}", number.Get());
	};

};