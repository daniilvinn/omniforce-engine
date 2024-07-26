#include "../AudioEngine.h"
#include <Log/Logger.h>
#include <Filesystem/Filesystem.h>

namespace Omni {

	void AudioEngine::Init()
	{
		s_Instance = new AudioEngine;
		OMNIFORCE_CORE_TRACE("Initialized audio engine");
	}

	void AudioEngine::Shutdown()
	{
		delete s_Instance;
	}

	void AudioEngine::StartPlaybackInlined(std::filesystem::path path)
	{
		ma_engine_play_sound(m_Engine, (FileSystem::GetWorkingDirectory() / path).string().c_str(), nullptr);
	}

	AudioEngine::AudioEngine()
	{
		m_Engine = new ma_engine;
		ma_result result = ma_engine_init(NULL, m_Engine);
		if (result != MA_SUCCESS)
			OMNIFORCE_CORE_CRITICAL("[AudioEngine]: failed to initialize");
		
	}

	AudioEngine::~AudioEngine()
	{
		ma_engine_uninit(m_Engine);
	}

}