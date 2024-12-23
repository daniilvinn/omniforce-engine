#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include "AudioSourceFlags.h"

#include <filesystem>

#include <miniaudio.h>

namespace Omni {

	struct OMNIFORCE_API AudioSourceConfig {
		std::filesystem::path path;
		AudioSourceFlags flags = AudioSourceFlags::NONE;
		float32 volume = 50.0f;
		bool looped = false;
		bool wait_on_load = false;
	};

	class OMNIFORCE_API AudioSource {
	public:
		AudioSource(const AudioSourceConfig& config);
		~AudioSource();

		void Play()	{ ma_sound_start(&m_Sound); }
		void Stop() { ma_sound_stop(&m_Sound); }
		bool Playing() const { return ma_sound_is_playing(&m_Sound); }
		bool Finished() const { return ma_sound_at_end(&m_Sound); }
		float32 CurrentTime()	{ return (float32)ma_sound_get_time_in_milliseconds(&m_Sound) * 0.001f; }

		uint32 GetSampleRate();
		float32 GetLength(); // in seconds
		float32 GetVolume();
		bool Looped() { return ma_sound_is_looping(&m_Sound); }

		void SetVolume(float32 volume);
		void SetLooped(bool looped) { ma_sound_set_looping(&m_Sound, looped); }

	private:
		ma_sound m_Sound;

	};

}