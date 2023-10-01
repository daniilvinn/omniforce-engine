#include "../AudioEngine.h"
#include <Log/Logger.h>

namespace Omni {

	void AudioEngine::Init()
	{
		s_Instance = new AudioEngine;
	}

	void AudioEngine::Shutdown()
	{
		delete s_Instance;
	}

	AudioEngine::AudioEngine()
	{
		ma_result result = ma_engine_init(NULL, &m_Engine);
		if (result != MA_SUCCESS)
			OMNIFORCE_CORE_CRITICAL("[AudioEngine]: failed to initialize");
		
	}

	AudioEngine::~AudioEngine()
	{
		ma_engine_uninit(&m_Engine);
	}

}