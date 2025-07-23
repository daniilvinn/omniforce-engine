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

		ImGui::Begin("Path Tracing Settings", &m_IsOpen);
		
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
	}

	void PathTracingSettingsPanel::SetContext(Scene* ctx)
	{
		EditorPanel::SetContext(ctx);
		m_SettingsChanged = false;
	}

	void PathTracingSettingsPanel::RenderCoreSettings()
	{
		ImGui::PushID("CoreSettings");
		
		// Max Bounces
		int maxBounces = static_cast<int>(m_CurrentSettings.MaxBounces);
		if (ImGui::SliderInt("Max Bounces", &maxBounces, 1, 16)) {
			m_CurrentSettings.MaxBounces = static_cast<uint32>(maxBounces);
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::InputInt("##MaxBouncesInput", &maxBounces, 1, 5, ImGuiInputTextFlags_EnterReturnsTrue)) {
			maxBounces = std::clamp(maxBounces, 1, 32);
			m_CurrentSettings.MaxBounces = static_cast<uint32>(maxBounces);
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Maximum number of light bounces per ray path");
		}

		// Samples Per Pixel
		int samplesPerPixel = static_cast<int>(m_CurrentSettings.SamplesPerPixel);
		if (ImGui::SliderInt("Samples Per Pixel", &samplesPerPixel, 1, 64)) {
			m_CurrentSettings.SamplesPerPixel = static_cast<uint32>(samplesPerPixel);
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::InputInt("##SamplesPerPixelInput", &samplesPerPixel, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue)) {
			samplesPerPixel = std::clamp(samplesPerPixel, 1, 256);
			m_CurrentSettings.SamplesPerPixel = static_cast<uint32>(samplesPerPixel);
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Number of samples per pixel (higher = less noise, slower)");
		}

		// Deterministic Seed
		if (ImGui::Checkbox("Deterministic Seed", &m_CurrentSettings.DeterministicSeed)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Use fixed seed for reproducible results");
		}

		// Fixed Seed (only show if deterministic is enabled)
		if (m_CurrentSettings.DeterministicSeed) {
			ImGui::Indent();
			int fixedSeed = static_cast<int>(m_CurrentSettings.FixedSeed);
			if (ImGui::SliderInt("Fixed Seed", &fixedSeed, 0, 1000)) {
				m_CurrentSettings.FixedSeed = static_cast<uint32>(fixedSeed);
				m_SettingsChanged = true;
			}
			ImGui::SameLine();
			if (ImGui::InputInt("##FixedSeedInput", &fixedSeed, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
				fixedSeed = std::clamp(fixedSeed, 0, 1000000);
				m_CurrentSettings.FixedSeed = static_cast<uint32>(fixedSeed);
				m_SettingsChanged = true;
			}
			ImGui::Unindent();
		}

		// Note: Accumulation limit is handled by the renderer, not in settings
		ImGui::Text("Accumulation Limit: %u", m_CurrentSettings.MaxAccumulatedFrameCount);
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Maximum frames to accumulate (fixed at 4096)");
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Maximum frames to accumulate (higher = better quality, slower convergence)");
		}

		// Reset Accumulation Button
		ImGui::Spacing();
		if (ImGui::Button("Reset Accumulation", ImVec2(-1, 0))) {
			auto* renderer = GetPathTracingRenderer();
			if (renderer) {
				renderer->ResetAccumulation();
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Reset the accumulated frame counter to start fresh");
		}

		ImGui::PopID();
	}

	void PathTracingSettingsPanel::RenderQualitySettings()
	{
		ImGui::PushID("QualitySettings");

		// Russian Roulette Threshold
		if (ImGui::SliderFloat("Russian Roulette Threshold", &m_CurrentSettings.RussianRouletteThreshold, 0.01f, 0.5f, "%.3f")) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::InputFloat("##RussianRouletteInput", &m_CurrentSettings.RussianRouletteThreshold, 0.001f, 0.01f, "%.4f", ImGuiInputTextFlags_EnterReturnsTrue)) {
			m_CurrentSettings.RussianRouletteThreshold = std::clamp(m_CurrentSettings.RussianRouletteThreshold, 0.001f, 1.0f);
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Threshold for probabilistic path termination (lower = more paths, slower)");
		}

		// Enable MIS
		if (ImGui::Checkbox("Enable MIS", &m_CurrentSettings.EnableMIS)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Multiple Importance Sampling for better light sampling");
		}

		// Enable NEE
		if (ImGui::Checkbox("Enable NEE", &m_CurrentSettings.EnableNEE)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Next Event Estimation for direct light sampling");
		}

		// Ray Epsilon
		if (ImGui::SliderFloat("Ray Epsilon", &m_CurrentSettings.RayEpsilon, 0.0001f, 0.01f, "%.4f")) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::InputFloat("##RayEpsilonInput", &m_CurrentSettings.RayEpsilon, 0.0001f, 0.001f, "%.6f", ImGuiInputTextFlags_EnterReturnsTrue)) {
			m_CurrentSettings.RayEpsilon = std::clamp(m_CurrentSettings.RayEpsilon, 0.00001f, 0.1f);
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Ray origin offset to prevent self-intersection");
		}

		// Enable Environment Lighting
		if (ImGui::Checkbox("Environment Lighting", &m_CurrentSettings.EnableEnvironmentLighting)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Enable sky/environment lighting");
		}

		ImGui::PopID();
	}

	void PathTracingSettingsPanel::RenderDebugSettings()
	{
		ImGui::PushID("DebugSettings");

		// Ray Debug Mode
		if (ImGui::Checkbox("Ray Debug Mode", &m_CurrentSettings.RayDebugMode)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Enable ray debugging visualization");
		}

		// Visualize Paths
		if (ImGui::Checkbox("Visualize Paths", &m_CurrentSettings.VisualizePaths)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Show ray paths in 3D space");
		}

		// Visualize Hit Points
		if (ImGui::Checkbox("Visualize Hit Points", &m_CurrentSettings.VisualizeHitPoints)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Display surface hit points");
		}

		// Visualize Materials
		if (ImGui::Checkbox("Visualize Materials", &m_CurrentSettings.VisualizeMaterials)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Show material properties (albedo, normal, etc.)");
		}

		// Show Performance Metrics
		if (ImGui::Checkbox("Show Performance Metrics", &m_CurrentSettings.GatherPerformanceMetrics)) {
			m_SettingsChanged = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Display real-time performance information");
		}

		ImGui::PopID();
	}

	void PathTracingSettingsPanel::RenderPerformanceMetrics()
	{
		ImGui::PushID("PerformanceMetrics");

		auto* renderer = GetPathTracingRenderer();
		if (!renderer) {
			ImGui::PopID();
			return;
		}

		const auto& settings = renderer->GetSettings();

		// Performance stats
		ImGui::Text("Rays per Second: %llu", 0);
		ImGui::Text("Convergence Rate: %.4f", 0);
		ImGui::Text("Current Samples: %u", 0);
		ImGui::Text("Total Rays: %llu", 0);

		// Progress bar for accumulation
		float progress = settings.MaxAccumulatedFrameCount > 0 ? 
			static_cast<float>(renderer->GetAccumulatedFrameCount()) / static_cast<float>(settings.MaxAccumulatedFrameCount) : 0.0f;
		
		ImGui::Spacing();
		ImGui::Text("Accumulation Progress:");
		ImGui::ProgressBar(progress, ImVec2(-1, 0), 
			fmt::format("{}/{}", renderer->GetAccumulatedFrameCount(), settings.MaxAccumulatedFrameCount).c_str());

		if (renderer->GetAccumulatedFrameCount() >= settings.MaxAccumulatedFrameCount) {
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Maximum accumulation reached");
		}

		ImGui::PopID();
	}

	void PathTracingSettingsPanel::RenderPresets()
	{
		ImGui::PushID("Presets");

		// Performance Preset
		if (ImGui::Button("Performance", ImVec2(-1, 0))) {
			PathTracingSettings preset;
			preset.MaxBounces = 2;
			preset.SamplesPerPixel = 1;
			preset.MaxAccumulatedFrameCount = 1024;
			preset.RussianRouletteThreshold = 0.2f;
			preset.EnableMIS = true;
			preset.EnableNEE = true;
			preset.RayEpsilon = 0.001f;
			preset.EnableEnvironmentLighting = true;
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

		// Quality Preset
		if (ImGui::Button("Quality", ImVec2(-1, 0))) {
			PathTracingSettings preset;
			preset.MaxBounces = 8;
			preset.SamplesPerPixel = 4;
			preset.MaxAccumulatedFrameCount = 8192;
			preset.RussianRouletteThreshold = 0.05f;
			preset.EnableMIS = true;
			preset.EnableNEE = true;
			preset.RayEpsilon = 0.0001f;
			preset.EnableEnvironmentLighting = true;
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

		// Debug Preset
		if (ImGui::Button("Debug", ImVec2(-1, 0))) {
			PathTracingSettings preset;
			preset.MaxBounces = 3;
			preset.SamplesPerPixel = 1;
			preset.MaxAccumulatedFrameCount = 2048;
			preset.RussianRouletteThreshold = 0.1f;
			preset.EnableMIS = true;
			preset.EnableNEE = true;
			preset.RayEpsilon = 0.001f;
			preset.EnableEnvironmentLighting = true;
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

		// Balanced Preset
		if (ImGui::Button("Balanced", ImVec2(-1, 0))) {
			PathTracingSettings preset;
			preset.MaxBounces = 4;
			preset.SamplesPerPixel = 2;
			preset.MaxAccumulatedFrameCount = 4096;
			preset.RussianRouletteThreshold = 0.1f;
			preset.EnableMIS = true;
			preset.EnableNEE = true;
			preset.RayEpsilon = 0.001f;
			preset.EnableEnvironmentLighting = true;
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
		ImGui::SetTooltip(fmt::format("Applied {} preset", presetName).c_str());
	}

} 