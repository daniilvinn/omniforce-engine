#pragma once

#include <Foundation/Macros.h>

namespace Cursed {

	class CURSED_API Subsystem {
	public:
		Subsystem() {};
		virtual ~Subsystem() {};

		virtual void Launch() = 0;
		virtual void Destroy() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnEvent() = 0;

	};

}