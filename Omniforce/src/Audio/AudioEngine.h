#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include "miniaudio.h"

namespace Omni {

	class OMNIFORCE_API AudioEngine {
	public:
		static void Init();
		static void Shutdown();
		static AudioEngine* Get() { return s_Instance; };

		ma_engine& GetEngine() { return m_Engine; }
		const ma_engine& GetEngine() const { return m_Engine; };


	private:
		AudioEngine();
		~AudioEngine();

		inline static AudioEngine* s_Instance = nullptr;
		ma_engine m_Engine;

	};

}