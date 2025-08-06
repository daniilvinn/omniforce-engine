#pragma once

#include "EditorPanel.h"
#include <Rendering/PathTracing/PathTracingSceneRenderer.h>
#include <Rendering/PathTracing/PathTracingSettings.h>

namespace Omni {

	class PathTracingSettingsPanel : public EditorPanel {
	public:
		PathTracingSettingsPanel(Scene* ctx);
		~PathTracingSettingsPanel() = default;

		void Update() override;
		void SetContext(Scene* ctx) override;

	private:
		void RenderCoreSettings();
		void RenderQualitySettings();
		void RenderDebugSettings();
		void RenderPerformanceMetrics();
		void RenderPresets();
		
		PathTracingSceneRenderer* GetPathTracingRenderer() const;
		void ApplyPreset(const PathTracingSettings& preset, const char* presetName);

	private:
		PathTracingSettings m_CurrentSettings;
		bool m_SettingsChanged = false;
		uint32 m_LastAccumulatedFrames = 0;
	};

} 