#include "../AudioEngine.h"
#include <Log/Logger.h>
#include <Filesystem/Filesystem.h>

namespace Omni {

	void AudioEngine::Init()
	{
		s_Instance = new AudioEngine;
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
			OMNIFORCE_CORE_CRITICAL("Failed to initialize audio engine");

		OMNIFORCE_CORE_INFO("Initialized audio engine");
		
	}

	AudioEngine::~AudioEngine()
	{
		ma_engine_uninit(m_Engine);
	}

}