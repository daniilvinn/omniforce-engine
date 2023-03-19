#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Allocator.h>

#include <functional>

namespace Cursed {

	class Event;

	using EventAlloc = TransientAllocator<false>;
	using EventCallback = std::function<void(Event*)>;

#define REGISTER_EVENT(e, cat)	inline static Event::Type GetStaticType() {return Event::Type::e;}\
								inline static Event::Kind GetStaticKind() {return Event::Kind::cat;}

// Thanks Yan TheCherno Chernikov for this macro <3
// https://github.com/TheCherno
#define CURSED_BIND_EVENT_FUNCTION(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
// --------------------------------------

	class CURSED_API Event {
	public:
		enum class Type : uint32 {
			WindowClose, WindowMoved, WindowResize, WindowFocus, WindowUnfocus,
			KeyPressed, KeyReleased, KeyTyped,
			MouseButtonPressed, MouseButtonHold, MouseButtonReleased, MouseMoved, MouseScrolled
		};
		enum class Kind : uint32 {
			Application = BIT(0),
			Keyboard = BIT(1),
			Mouse = BIT(2)
		};

		inline Type GetType() const { return m_Type; }

	protected:
		Event() {};
		Event(Event::Type type) : m_Type(type) {};
		Type m_Type;
		Kind m_Kind;
	};

	class EventDispatcher
	{
		template <typename T>
		using EventFunction = std::function<bool(T*)>;
	public:
		// Constructor
		EventDispatcher(Event* event)
			: m_Event(event) {}

		template<typename T>
		bool Dispatch(EventFunction<T> function)
		{
			if (m_Event->GetType() == T::GetStaticType())
			{
				return function(static_cast<T*>(m_Event));;
			}
			return false;
		}
	private:
		Event* m_Event;
	};

}