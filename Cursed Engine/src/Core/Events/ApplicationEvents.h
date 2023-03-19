#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include "Event.h"

namespace Cursed {

	class CURSED_API WindowResizeEvent : public Event {
	public:
		WindowResizeEvent(vec2<int32> resolution) : m_Resolution(resolution), Event(Event::Type::WindowResize) {};

		REGISTER_EVENT(WindowResize, Application);

		inline vec2<int32> GetResolution() const { return m_Resolution; }

	private:
		vec2<int32> m_Resolution;
	};

	class CURSED_API WindowCloseEvent : public Event {
	public:
		WindowCloseEvent() : Event(Event::Type::WindowClose) {}

		REGISTER_EVENT(WindowClose, Application);
	};

}