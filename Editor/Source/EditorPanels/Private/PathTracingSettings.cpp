#include "../PathTracingSettings.h"
#include <Rendering/ISceneRenderer.h>
#include <Rendering/PathTracing/PathTracingSceneRenderer.h>
#include <Rendering/PathTracing/PathTracingSettings.h>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <algorithm>

namespace Omni {

	PathTracingSettingsPanel::PathTracingSettingsPanel(Scene* ctx)
		: EditorPanel(ctx)
	{
		m_IsOpen = true;
	}

	void PathTracingSettingsPanel::Update()
	{
		if (!m_IsOpen) return;

		// Set window padding to 0 for cleaner look
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		// Add some cell padding for better readability
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));

		ImGui::Begin("Path Tracing Settings", &m_IsOpen);
		
		// Add some top spacing since we removed window padding
		ImGui::Spacing();
		
		// Check if we have a path tracing renderer
		auto* renderer = GetPathTracingRenderer();
		if (!renderer) {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Path Tracing renderer not active");
			ImGui::Text("Switch to Path Tracing mode to access these settings");
			ImGui::End();
			return;
		}

		// Sync current settings from renderer
		m_CurrentSettings = renderer->GetSettings();
		
		// Main settings sections
		if (ImGui::CollapsingHeader("Core Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			RenderCoreSettings();
		}
		
		if (ImGui::CollapsingHeader("Quality Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			RenderQualitySettings();
		}
		
		if (ImGui::CollapsingHeader("Debug & Visualization")) {
			RenderDebugSettings();
		}
		
		if (ImGui::CollapsingHeader("Performance Metrics")) {
			RenderPerformanceMetrics();
		}
		
		if (ImGui::CollapsingHeader("Presets")) {
			RenderPresets();
		}

		// Apply changes if any were made
		if (m_SettingsChanged) {
			renderer->SetSettings(m_CurrentSettings);
			m_SettingsChanged = false;
		}

		ImGui::End();
		
		// Pop the style variables
		ImGui::PopStyleVar(2);
	}

	void PathTracingSettingsPanel::SetContext(Scene* ctx)
	{
		EditorPanel::SetContext(ctx);
		m_SettingsChanged = false;
	}

	void PathTracingSettingsPanel::RenderCoreSettings()
	{
		ImGui::PushID("CoreSettings");
		
		ImGui::TextUnformatted("Core Settings");
		ImGui::Separator();
		ImGui::Spacing();
		
		if (ImGui::BeginTable("CoreSettingsTable", 3, 
			ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.37f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.58f);
			ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.05f);

			// Max Bounces
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Max Bounces");
			
			ImGui::TableSetColumnIndex(1);
			ImGui::PushItemWidth(200.0f);
			int maxBounces = static_cast<int>(m_CurrentSettings.MaxBounces);
			if (ImGui::DragInt("##MaxBounces", &maxBounces, 1.0f, 1, 32)) {
				maxBounces = std::clamp(maxBounces, 1, 32);
				m_CurrentSettings.MaxBounces = static_cast<uint32>(maxBounces);
				m_SettingsChanged = true;
			}
			ImGui::PopItemWidth();
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Maximum number of light bounces per ray path");
			}

			// Samples Per Pixel
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Samples Per Pixel");
			
			ImGui::TableSetColumnIndex(1);
			ImGui::PushItemWidth(200.0f);
			int samplesPerPixel = static_cast<int>(m_CurrentSettings.SamplesPerPixel);
			if (ImGui::DragInt("##SamplesPerPixel", &samplesPerPixel, 1.0f, 1, 256)) {
				samplesPerPixel = std::clamp(samplesPerPixel, 1, 256);
				m_CurrentSettings.SamplesPerPixel = static_cast<uint32>(samplesPerPixel);
				m_SettingsChanged = true;
			}
			ImGui::PopItemWidth();
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Number of samples per pixel (higher = less noise, slower)");
			}

			// Deterministic Seed
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Deterministic Seed");
			
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Checkbox("##DeterministicSeed", &m_CurrentSettings.DeterministicSeed)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Use fixed seed for reproducible results");
			}

			// Fixed Seed (only show if deterministic is enabled)
			if (m_CurrentSettings.DeterministicSeed) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("  Fixed Seed");
				
				ImGui::TableSetColumnIndex(1);
				ImGui::PushItemWidth(200.0f);
				int fixedSeed = static_cast<int>(m_CurrentSettings.FixedSeed);
				if (ImGui::DragInt("##FixedSeed", &fixedSeed, 1.0f, 0, 1000000)) {
					fixedSeed = std::clamp(fixedSeed, 0, 1000000);
					m_CurrentSettings.FixedSeed = static_cast<uint32>(fixedSeed);
					m_SettingsChanged = true;
				}
				ImGui::PopItemWidth();
				
				ImGui::TableSetColumnIndex(2);
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Seed value for deterministic rendering");
				}
			}

			// Enable Sky Light
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Enable Sky Light");
			
			ImGui::TableSetColumnIndex(1);
			if(ImGui::Checkbox("##EnableSkyLight", &m_CurrentSettings.EnableSkyLight)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Enable environment lighting from sky");
			}

			ImGui::EndTable();
		}

		// Sky light settings (separate section when enabled)
		if(m_CurrentSettings.EnableSkyLight) {
			ImGui::Spacing();
			ImGui::TextUnformatted("Sky Light Configuration");
			ImGui::Separator();
			ImGui::Spacing();
			
			if (ImGui::BeginTable("SkyLightTable", 3, 
				ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.37f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.58f);
				ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.05f);

				// Sky Color
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Sky Color");
				
				ImGui::TableSetColumnIndex(1);
				if (ImGui::ColorEdit3("##SkyColor", (float*)&m_CurrentSettings.SkyColor,
									ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
					m_SettingsChanged = true;
				}
				
				ImGui::TableSetColumnIndex(2);
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Color of the sky light");
				}

				// Sky Intensity
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Sky Intensity");
				
				ImGui::TableSetColumnIndex(1);
				ImGui::PushItemWidth(200.0f);
				if(ImGui::DragFloat("##SkyIntensity", &m_CurrentSettings.SkyLightIntensity, 0.01f, 0.0f, 100.0f)) {
					m_SettingsChanged = true;
				}
				ImGui::PopItemWidth();
				
				ImGui::TableSetColumnIndex(2);
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Intensity multiplier for sky light");
				}

				// Sun Direction
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Sun Direction");
				
				ImGui::TableSetColumnIndex(1);
				ImGui::PushItemWidth(200.0f);
				if(ImGui::DragFloat3("##SunDirection", (float*)&m_CurrentSettings.SunDirection, 0.01f, -1.0f, 1.0f)) {
					m_CurrentSettings.SunDirection = glm::normalize(m_CurrentSettings.SunDirection);
					m_SettingsChanged = true;
				}
				ImGui::PopItemWidth();
				
				ImGui::TableSetColumnIndex(2);
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Direction vector for sun light");
				}

				ImGui::EndTable();
			}
		}

		// Star settings
		ImGui::Spacing();
		ImGui::TextUnformatted("Star Field Settings");
		ImGui::Separator();
		ImGui::Spacing();
		
		if (ImGui::BeginTable("StarSettingsTable", 3, 
			ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.37f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.58f);
			ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.05f);

			// Star Generation Seed
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Generation Seed");
			
			ImGui::TableSetColumnIndex(1);
			ImGui::PushItemWidth(200.0f);
			if(ImGui::DragInt("##StarSeed", (int*)&m_CurrentSettings.StarGenerationSeed, 1.0f, 0, 256)) {
				m_SettingsChanged = true;
			}
			ImGui::PopItemWidth();
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Seed for procedural star generation");
			}

			// Star Density
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Star Density");
			
			ImGui::TableSetColumnIndex(1);
			ImGui::PushItemWidth(200.0f);
			if(ImGui::DragInt("##StarDensity", (int*)&m_CurrentSettings.StarDensity, 1.0f, 1, 32)) {
				m_SettingsChanged = true;
			}
			ImGui::PopItemWidth();
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Density of stars in the sky");
			}

			ImGui::EndTable();
		}

		ImGui::PopID();
	}

	void PathTracingSettingsPanel::RenderQualitySettings()
	{
		ImGui::PushID("QualitySettings");

		if (ImGui::BeginTable("QualitySettingsTable", 3, 
			ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.37f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.58f);
			ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.05f);

			// Max Accumulated Frames
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Max Accumulated Frames");
			
			ImGui::TableSetColumnIndex(1);
			ImGui::PushItemWidth(200.0f);
			if(ImGui::DragInt("##MaxAccumulatedFrames", (int*)&m_CurrentSettings.MaxAccumulatedFrameCount, 1.0f, 1, glm::pow(2, 14))) {
				m_SettingsChanged = true;
			}
			ImGui::PopItemWidth();
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Maximum frames to accumulate for noise reduction");
			}

			// Russian Roulette Threshold
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Russian Roulette Threshold");
			
			ImGui::TableSetColumnIndex(1);
			ImGui::PushItemWidth(200.0f);
			if (ImGui::DragFloat("##RussianRoulette", &m_CurrentSettings.RussianRouletteThreshold, 0.001f, 0.001f, 1.0f, "%.4f")) {
				m_CurrentSettings.RussianRouletteThreshold = std::clamp(m_CurrentSettings.RussianRouletteThreshold, 0.001f, 1.0f);
				m_SettingsChanged = true;
			}
			ImGui::PopItemWidth();
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Threshold for probabilistic path termination (lower = more paths, slower)");
			}

			// Enable MIS
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Enable MIS");
			
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Checkbox("##EnableMIS", &m_CurrentSettings.EnableMIS)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Multiple Importance Sampling for better light sampling");
			}

			// Enable NEE
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Enable NEE");
			
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Checkbox("##EnableNEE", &m_CurrentSettings.EnableNEE)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Next Event Estimation for direct light sampling");
			}

			// Ray Epsilon
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Ray Epsilon");
			
			ImGui::TableSetColumnIndex(1);
			ImGui::PushItemWidth(200.0f);
			if (ImGui::DragFloat("##RayEpsilon", &m_CurrentSettings.RayEpsilon, 0.0001f, 0.00001f, 0.1f, "%.6f")) {
				m_CurrentSettings.RayEpsilon = std::clamp(m_CurrentSettings.RayEpsilon, 0.00001f, 0.1f);
				m_SettingsChanged = true;
			}
			ImGui::PopItemWidth();
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Ray origin offset to prevent self-intersection");
			}

			ImGui::EndTable();
		}

		ImGui::PopID();
	}

	void PathTracingSettingsPanel::RenderDebugSettings()
	{
		ImGui::PushID("DebugSettings");

		if (ImGui::BeginTable("DebugSettingsTable", 3, 
			ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.37f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.58f);
			ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch, 0.05f);

			// Ray Debug Mode
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Ray Debug Mode");
			
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Checkbox("##RayDebugMode", &m_CurrentSettings.RayDebugMode)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Enable ray debugging visualization");
			}

			// Visualize Paths
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Visualize Paths");
			
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Checkbox("##VisualizePaths", &m_CurrentSettings.VisualizePaths)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Show ray paths in 3D space");
			}

			// Visualize Hit Points
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Visualize Hit Points");
			
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Checkbox("##VisualizeHitPoints", &m_CurrentSettings.VisualizeHitPoints)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Display surface hit points");
			}

			// Visualize Materials
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Visualize Materials");
			
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Checkbox("##VisualizeMaterials", &m_CurrentSettings.VisualizeMaterials)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Show material properties (albedo, normal, etc.)");
			}

			// Show Performance Metrics
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Gather Performance Metrics");
			
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Checkbox("##GatherPerformanceMetrics", &m_CurrentSettings.GatherPerformanceMetrics)) {
				m_SettingsChanged = true;
			}
			
			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Display real-time performance information");
			}

			ImGui::EndTable();
		}

		ImGui::PopID();
	}

	void PathTracingSettingsPanel::RenderPerformanceMetrics()
	{
		ImGui::PushID("PerformanceMetrics");

		auto* renderer = GetPathTracingRenderer();
		if (!renderer) {
			ImGui::Text("No path tracing renderer available");
			ImGui::PopID();
			return;
		}

		const auto& settings = renderer->GetSettings();

		if (ImGui::BeginTable("PerformanceTable", 2, 
			ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 0.45f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.55f);

			// Rays per Second
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Rays per Second");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%llu", 0ull);

			// Convergence Rate
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Convergence Rate");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.4f", 0.0f);

			// Current Samples
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Current Samples");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u", 0u);

			// Total Rays
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Total Rays");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%llu", 0ull);

			// Current Frame Count
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Accumulated Frames");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%u / %u", renderer->GetAccumulatedFrameCount(), settings.MaxAccumulatedFrameCount);

			ImGui::EndTable();
		}

		// Progress bar for accumulation
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("Accumulation Progress");
		ImGui::Spacing();
		
		float progress = settings.MaxAccumulatedFrameCount > 0 ? 
			static_cast<float>(renderer->GetAccumulatedFrameCount()) / static_cast<float>(settings.MaxAccumulatedFrameCount) : 0.0f;
		
		ImGui::ProgressBar(progress, ImVec2(-1, 0), 
			fmt::format("{}/{}", renderer->GetAccumulatedFrameCount(), settings.MaxAccumulatedFrameCount).c_str());

		if (renderer->GetAccumulatedFrameCount() >= settings.MaxAccumulatedFrameCount) {
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Maximum accumulation reached");
		}

		ImGui::PopID();
	}

	void PathTracingSettingsPanel::RenderPresets()
	{
		ImGui::PushID("Presets");

		ImGui::TextUnformatted("Quick Configuration Presets");
		ImGui::Separator();
		ImGui::Spacing();

		// Use a 2x2 grid layout for the preset buttons
		if (ImGui::BeginTable("PresetsTable", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter))
		{
			// Performance Preset
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			if (ImGui::Button("Performance##Preset", ImVec2(-1, 40))) {
				PathTracingSettings preset;
				preset.MaxBounces = 2;
				preset.SamplesPerPixel = 1;
				preset.MaxAccumulatedFrameCount = 1024;
				preset.RussianRouletteThreshold = 0.2f;
				preset.EnableMIS = true;
				preset.EnableNEE = true;
				preset.RayEpsilon = 0.001f;
				preset.DeterministicSeed = false;
				preset.FixedSeed = 42;
				preset.RayDebugMode = false;
				preset.VisualizePaths = false;
				preset.VisualizeHitPoints = false;
				preset.VisualizeMaterials = false;
				preset.GatherPerformanceMetrics = true;
				preset.AdaptiveSampling = false;
				preset.EnableDenoising = false;
				preset.TemporalFilterStrength = 1.0f;
				
				ApplyPreset(preset, "Performance");
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Fast rendering settings\nLow bounces, minimal samples");
			}

			// Quality Preset
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Button("Quality##Preset", ImVec2(-1, 40))) {
				PathTracingSettings preset;
				preset.MaxBounces = 8;
				preset.SamplesPerPixel = 4;
				preset.MaxAccumulatedFrameCount = 8192;
				preset.RussianRouletteThreshold = 0.05f;
				preset.EnableMIS = true;
				preset.EnableNEE = true;
				preset.RayEpsilon = 0.0001f;
				preset.DeterministicSeed = false;
				preset.FixedSeed = 42;
				preset.RayDebugMode = false;
				preset.VisualizePaths = false;
				preset.VisualizeHitPoints = false;
				preset.VisualizeMaterials = false;
				preset.GatherPerformanceMetrics = true;
				preset.AdaptiveSampling = true;
				preset.EnableDenoising = true;
				preset.TemporalFilterStrength = 0.8f;
				
				ApplyPreset(preset, "Quality");
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("High quality settings\nMore bounces, advanced sampling");
			}

			// Debug Preset
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			if (ImGui::Button("Debug##Preset", ImVec2(-1, 40))) {
				PathTracingSettings preset;
				preset.MaxBounces = 3;
				preset.SamplesPerPixel = 1;
				preset.MaxAccumulatedFrameCount = 2048;
				preset.RussianRouletteThreshold = 0.1f;
				preset.EnableMIS = true;
				preset.EnableNEE = true;
				preset.RayEpsilon = 0.001f;
				preset.DeterministicSeed = true;
				preset.RayDebugMode = true;
				preset.VisualizePaths = true;
				preset.VisualizeHitPoints = true;
				preset.VisualizeMaterials = true;
				preset.GatherPerformanceMetrics = true;
				preset.AdaptiveSampling = false;
				preset.EnableDenoising = false;
				preset.TemporalFilterStrength = 1.0f;
				
				ApplyPreset(preset, "Debug");
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Debug visualization settings\nEnables all debug modes");
			}

			// Balanced Preset
			ImGui::TableSetColumnIndex(1);
			if (ImGui::Button("Balanced##Preset", ImVec2(-1, 40))) {
				PathTracingSettings preset;
				preset.MaxBounces = 4;
				preset.SamplesPerPixel = 2;
				preset.MaxAccumulatedFrameCount = 4096;
				preset.RussianRouletteThreshold = 0.1f;
				preset.EnableMIS = true;
				preset.EnableNEE = true;
				preset.RayEpsilon = 0.001f;
				preset.DeterministicSeed = false;
				preset.FixedSeed = 42;
				preset.RayDebugMode = false;
				preset.VisualizePaths = false;
				preset.VisualizeHitPoints = false;
				preset.VisualizeMaterials = false;
				preset.GatherPerformanceMetrics = true;
				preset.AdaptiveSampling = false;
				preset.EnableDenoising = false;
				preset.TemporalFilterStrength = 1.0f;
				
				ApplyPreset(preset, "Balanced");
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Balanced settings\nGood quality/performance trade-off");
			}

			ImGui::EndTable();
		}

		ImGui::PopID();
	}

	PathTracingSceneRenderer* PathTracingSettingsPanel::GetPathTracingRenderer() const
	{
		if (!m_Context) return nullptr;
		
		auto renderer = m_Context->GetRenderer();
		if (!renderer) return nullptr;
		
		if (renderer->GetRenderMode() != SceneRendererMode::PATH_TRACING) {
			return nullptr;
		}
		
		return (PathTracingSceneRenderer*)renderer.Raw();
	}

	void PathTracingSettingsPanel::ApplyPreset(const PathTracingSettings& preset, const char* presetName)
	{
		m_CurrentSettings = preset;
		m_SettingsChanged = true;
		
		// Show a brief notification
        ImGui::SetTooltip("%s", fmt::format("Applied {} preset", presetName).c_str());
	}

} 