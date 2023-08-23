#pragma once

#include <Scene/Entity.h>

namespace Omni {

	struct PendingCallbackInfo {
		Entity entity;
		std::string method;
		std::vector<void*> args;
	};

}