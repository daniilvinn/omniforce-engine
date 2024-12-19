#pragma once

#include <Core/Input/KeyCode.h>

namespace Omni {

	class OMNIFORCE_API MouseMovedEvent : public Event {
	public:
		MouseMovedEvent() = delete;
		MouseMovedEvent(vec2<float32> pos) : m_Position(pos), Event(Event::Type::MouseMoved) {};

		REGISTER_EVENT(MouseMoved, Mouse)

		vec2<float32> GetPosition() const { return m_Position; }

	private:
		vec2<float32> m_Position;
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
		MouseScrolledEvent(vec2<float32> axis) : m_Axis(axis), Event(Event::Type::MouseScrolled) {};

		REGISTER_EVENT(MouseScrolled, Mouse)

		inline const vec2<float32> GetAxis() const { return m_Axis; }

	private:
		vec2<float32> m_Axis;
	};

}