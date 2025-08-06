#pragma once

#include <Foundation/Common.h>
#include <Core/Events/Event.h>

namespace Omni {

	class OMNIFORCE_API WindowResizeEvent : public Event {
	public:
		WindowResizeEvent(ivec2 resolution) : m_Resolution(resolution), Event(Event::Type::WindowResize) {};

		REGISTER_EVENT(WindowResize, Application);

		inline ivec2 GetResolution() const { return m_Resolution; }

	private:
		ivec2 m_Resolution;
	};

	class OMNIFORCE_API WindowCloseEvent : public Event {
	public:
		WindowCloseEvent() : Event(Event::Type::WindowClose) {}

		REGISTER_EVENT(WindowClose, Application);
	};

}