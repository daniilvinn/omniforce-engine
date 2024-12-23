#pragma once

#include <Core/Input/KeyCode.h>

namespace Omni {

	class OMNIFORCE_API MouseMovedEvent : public Event {
	public:
		MouseMovedEvent() = delete;
		MouseMovedEvent(fvec2 pos) : m_Position(pos), Event(Event::Type::MouseMoved) {};

		REGISTER_EVENT(MouseMoved, Mouse)

		fvec2 GetPosition() const { return m_Position; }

	private:
		fvec2 m_Position;
	};

	class OMNIFORCE_API MouseButtonPressedEvent : public Event {
	public:
		MouseButtonPressedEvent() = delete;
		MouseButtonPressedEvent(ButtonCode button) : m_ButtonCode(button), Event(Event::Type::MouseButtonPressed) {};

		inline const ButtonCode GetButton() const { return m_ButtonCode; }

		REGISTER_EVENT(MouseButtonPressed, Mouse)

	private:
		ButtonCode m_ButtonCode;
	};

	class OMNIFORCE_API MouseButtonReleasedEvent : public Event {
	public:
		MouseButtonReleasedEvent() = delete;
		MouseButtonReleasedEvent(ButtonCode button) : m_ButtonCode(button), Event(Event::Type::MouseButtonReleased) {};

		REGISTER_EVENT(MouseButtonReleased, Mouse)

		inline const ButtonCode GetButton() const { return m_ButtonCode; }
	private:
		ButtonCode m_ButtonCode;
	};

	class OMNIFORCE_API MouseScrolledEvent : public Event {
	public:
		MouseScrolledEvent() = delete;
		MouseScrolledEvent(fvec2 axis) : m_Axis(axis), Event(Event::Type::MouseScrolled) {};

		REGISTER_EVENT(MouseScrolled, Mouse)

		inline const fvec2 GetAxis() const { return m_Axis; }

	private:
		fvec2 m_Axis;
	};

}