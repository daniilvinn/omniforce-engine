#pragma once

#include <memory>

namespace Cursed {

	template <typename T>
	using Scope = std::unique_ptr<T>;

	template <typename T>
	using Shared = std::shared_ptr<T>;

	template <typename T1, typename T2>
	Shared<T1> ShareAs(Shared<T2>& ptr)
	{
		return std::static_pointer_cast<T1>(ptr);
	};

#if 0
	template <typename T>
	class CURSED_API Scope : public std::unique_ptr<T> {
	public:

		template <typename... Args>
		static Scope<T> Create(Args&&... args) {
			return (Scope<T>)std::make_unique<T>(std::forward<Args>(args)...);
		}

		template <typename T1>
		constexpr Scope<T1> Cast()
		{
			return std::static_pointer_cast<T1>(this);
		};
	};

	template <typename T>
	class CURSED_API Shared : public std::shared_ptr<T> {
	public:
		template <typename... Args>
		static Shared<T> Create(Args&&... args) {
			return (Shared<T>)std::make_shared<T>(std::forward<Args>(args)...);
		}

		template <typename T1>
		constexpr Shared<T1> Cast()
		{
			return std::static_pointer_cast<T1>(this);
		};
	};
#endif
	
}