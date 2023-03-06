#pragma once

#include "Event.h"

#include <Core/Input/KeyCode.h>

namespace Cursed {

	class CURSED_API KeyPressedEvent : public Event {
	public:
		KeyPressedEvent() = delete;
		KeyPressedEvent(KeyCode key, uint32 repeat) : m_KeyCode(key), m_Repeat(repeat), Event(Event::Type::KeyPressed) {};

		REGISTER_EVENT(KeyPressed, Keyboard)

		inline KeyCode GetKey() const { return m_KeyCode; }
		inline uint32 GetRepeatCount() const { m_Repeat; }

	private:
		KeyCode m_KeyCode;
		uint32 m_Repeat;
	};

	class CURSED_API KeyReleasedEvent : public Event {
	public:
		KeyReleasedEvent() = delete;
		KeyReleasedEvent(KeyCode key) : m_KeyCode(key), m_Unused(0), Event(Event::Type::KeyReleased) {};

		REGISTER_EVENT(KeyReleased, Keyboard)

		inline KeyCode GetKey() const { return m_KeyCode; }

	private:
		KeyCode m_KeyCode;
		uint32 m_Unused; // used to align all event objects' size to 16 byte
	};

	class CURSED_API KeyTypedEvent : public Event {
	public:
		KeyTypedEvent() = delete;
		KeyTypedEvent(KeyCode key) : m_KeyCode(key), Event(Event::Type::KeyTyped) {};

		REGISTER_EVENT(KeyTyped, Keyboard)

		inline KeyCode GetKey() const { return m_KeyCode; }

	private:
		KeyCode m_KeyCode;
	};

}