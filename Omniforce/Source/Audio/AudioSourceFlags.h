#pragma once 

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include "miniaudio.h"

namespace Omni {

	enum class OMNIFORCE_API AudioSourceFlags : uint32 {
		NONE					= NULL,
		ENABLE_STREAMING		= MA_SOUND_FLAG_STREAM,
		NO_PITCH				= MA_SOUND_FLAG_NO_PITCH,
		NO_SPATIALIZATION		= MA_SOUND_FLAG_NO_SPATIALIZATION
	};

}