#include "../AudioSource.h"
#include "../AudioEngine.h"
#include "../miniaudio.h"

namespace Omni {

	AudioSource::AudioSource(const AudioSourceConfig& config)
	{
		AudioEngine* audio_engine = AudioEngine::Get();
		ma_engine& engine = audio_engine->GetEngine();

		ma_fence fence;
		ma_sound_init_from_file(&engine, config.path.string().c_str(), (uint32)config.flags, nullptr, &fence, &m_Sound);
		if (config.wait_on_load)
			ma_fence_wait(&fence);
		ma_sound_set_volume(&m_Sound, config.volume * 0.1f);
		ma_sound_set_looping(&m_Sound, config.looped);
	}

	AudioSource::~AudioSource()
	{
		ma_sound_uninit(&m_Sound);
	}

	uint32 AudioSource::GetSampleRate()
	{
		float32 time_seconds;
		uint64 time_pcm_frames;

		ma_sound_get_length_in_seconds(&m_Sound, &time_seconds);
		ma_sound_get_length_in_pcm_frames(&m_Sound, &time_pcm_frames);

		return time_pcm_frames / time_seconds;
	}

	float32 AudioSource::GetLength()
	{
		float32 result;
		ma_sound_get_length_in_seconds(&m_Sound, &result);
		return result;
	}

	float32 AudioSource::GetVolume()
	{
		float32 volume = ma_sound_get_volume(&m_Sound);
		return volume * 10.0f;
	}

	void AudioSource::SetVolume(float32 volume)
	{
		ma_sound_set_volume(&m_Sound, volume);
	}

}