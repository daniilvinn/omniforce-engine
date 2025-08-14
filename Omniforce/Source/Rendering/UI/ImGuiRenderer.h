#pragma once

#include <Foundation/Common.h>

#include <RHI/RHICommon.h>
#include <RHI/Image.h>

#include <imgui.h>
#include <glm/glm.hpp>
#include <string>
#include <functional>

namespace Omni {
	class OMNIFORCE_API ImGuiRenderer {
	public:

		static ImGuiRenderer* Create();

		virtual void Launch(void* window_handle) = 0;
		virtual void Destroy() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void OnRender() = 0;
	};

	// ============================================================================
	// UI Abstraction Layer
	// ============================================================================
	
	// Forward declarations
	class UITable;
	class UIPropertyField;
	class UIPanel;
	
	// UI Theme and Style Management
	enum class UITheme {
		Dark,
		Light,
		Custom
	};
	
	enum class UITableStyle {
		Default,        // Standard table with borders and row backgrounds
		Compact,        // Minimal spacing, no row backgrounds
		Properties,     // Label-value pairs with vertical borders
		DataGrid,       // Full-featured data grid with sorting/filtering
		Logs           // Specialized for log display
	};
	
	enum class UIPropertyType {
		Float,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
		Bool,
		String,
		Color,
		Asset,
		Enum
	};
	
	// Table Column Configuration
	struct UITableColumn {
		std::string label;
		ImGuiTableColumnFlags flags = ImGuiTableColumnFlags_None;
		float width = 0.0f;
		float width_ratio = 0.0f;
		
		UITableColumn() = default;
		UITableColumn(const char* label, ImGuiTableColumnFlags flags = ImGuiTableColumnFlags_None, float width = 0.0f)
			: label(label), flags(flags), width(width) {}
		UITableColumn(const char* label, float width_ratio)
			: label(label), flags(ImGuiTableColumnFlags_WidthStretch), width_ratio(width_ratio) {}
	};
	
	// Property Field Configuration
	struct UIPropertyConfig {
		float speed = 1.0f;
		float min_value = -FLT_MAX;
		float max_value = FLT_MAX;
		const char* format = nullptr;
		const char* tooltip = nullptr;
		bool read_only = false;
		std::function<void()> on_changed = nullptr;
	};
	
	namespace UI {
		// ============================================================================
		// Core UI System
		// ============================================================================
		
		// Initialize and shutdown the UI system
		void OMNIFORCE_API Initialize();
		void OMNIFORCE_API Shutdown();
		
		// Theme management
		void OMNIFORCE_API ApplyTheme(UITheme theme);
		void OMNIFORCE_API SetCustomTheme(const ImGuiStyle& style);
		
		// ============================================================================
		// Table System - The main focus for consistent rendering
		// ============================================================================
		
		// Create a table with consistent styling
		class OMNIFORCE_API Table {
		public:
			Table(const char* id, int columns, UITableStyle style = UITableStyle::Default);
			~Table();
			
			// Setup columns
			void SetupColumn(const char* label, ImGuiTableColumnFlags flags = ImGuiTableColumnFlags_None, float width = 0.0f);
			void SetupColumn(const char* label, float width_ratio);
			void SetupColumns(const std::vector<UITableColumn>& columns);
			
			// Row management
			void NextRow();
			void NextColumn();
			void SetColumnIndex(int column);
			
			// Cell content helpers
			void CellText(const char* text);
			void CellTextFmt(const char* fmt, ...);
			void CellTextColored(const ImVec4& color, const char* text);
			void CellButton(const char* label, const ImVec2& size = ImVec2(0, 0));
			void CellCheckbox(const char* label, bool* value);
			void CellDragFloat(const char* label, float* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");
			void CellDragFloat3(const char* label, float* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");
			void CellInputText(const char* label, char* buffer, size_t buffer_size, ImGuiInputTextFlags flags = 0);
			
			// Header row
			void HeadersRow();
			
			// Utility
			bool IsValid() const { return m_IsValid; }
			
		private:
			const char* m_Id;
			int m_Columns;
			UITableStyle m_Style;
			bool m_IsValid;
			std::vector<UITableColumn> m_ColumnConfigs;
			
			// Style tracking for proper cleanup
			struct StyleStackEntry {
				int vars_pushed = 0;
				int colors_pushed = 0;
			} m_StyleEntry;
			
			void ApplyTableStyle();
			ImGuiTableFlags GetTableFlags() const;
		};
		
		// Convenience functions for common table patterns
		bool OMNIFORCE_API BeginPropertyTable(const char* id, bool* open = nullptr);
		void OMNIFORCE_API EndPropertyTable();
		
		bool OMNIFORCE_API BeginDataTable(const char* id, const std::vector<UITableColumn>& columns, bool* open = nullptr);
		void OMNIFORCE_API EndDataTable();
		
		bool OMNIFORCE_API BeginLogTable(const char* id, bool* open = nullptr);
		void OMNIFORCE_API EndLogTable();
		
		// ============================================================================
		// Property Field System
		// ============================================================================
		
		// Property field rendering with consistent styling
		bool OMNIFORCE_API PropertyField(const char* label, float& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, glm::vec2& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, glm::vec3& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, glm::vec4& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, int& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, glm::ivec2& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, glm::ivec3& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, glm::ivec4& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, bool& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, std::string& value, const UIPropertyConfig& config = {});
		bool OMNIFORCE_API PropertyField(const char* label, ImVec4& value, const UIPropertyConfig& config = {});
		
		// Asset property field with drag & drop support
		bool OMNIFORCE_API PropertyFieldAsset(const char* label, std::string& asset_path, const char* file_filter = nullptr, const UIPropertyConfig& config = {});
		
		// Enum property field
		template<typename T>
		bool PropertyFieldEnum(const char* label, T& value, const std::vector<std::pair<T, const char*>>& enum_values, const UIPropertyConfig& config = {});
		
		// ============================================================================
		// Panel System
		// ============================================================================
		
		// Panel management with consistent styling
		class OMNIFORCE_API Panel {
		public:
			Panel(const char* name, bool* open = nullptr, ImGuiWindowFlags flags = 0);
			~Panel();
			
			bool IsOpen() const { return m_IsOpen; }
			void SetOpen(bool open) { if (m_OpenPtr) *m_OpenPtr = open; }
			
		private:
			const char* m_Name;
			bool* m_OpenPtr;
			bool m_IsOpen;
			ImGuiWindowFlags m_Flags;
		};
		
		// Convenience panel functions
		bool OMNIFORCE_API BeginPanel(const char* name, bool* open = nullptr, ImGuiWindowFlags flags = 0);
		void OMNIFORCE_API EndPanel();
		
		// ============================================================================
		// Layout and Spacing Helpers
		// ============================================================================
		
		// Consistent spacing and layout
		void OMNIFORCE_API PushCompactStyle();
		void OMNIFORCE_API PopCompactStyle();
		
		void OMNIFORCE_API PushPanelStyle();
		void OMNIFORCE_API PopPanelStyle();
		
		void OMNIFORCE_API PushTableStyle(UITableStyle style);
		void OMNIFORCE_API PopTableStyle();
		
		// ============================================================================
		// Utility Functions
		// ============================================================================
		
		// Text rendering with consistent styling
		void OMNIFORCE_API Text(const char* text);
		void OMNIFORCE_API TextColored(const ImVec4& color, const char* text);
		void OMNIFORCE_API TextDisabled(const char* text);
		void OMNIFORCE_API TextWrapped(const char* text);
		
		// Button rendering with consistent styling
		bool OMNIFORCE_API Button(const char* label, const ImVec2& size = ImVec2(0, 0));
		bool OMNIFORCE_API SmallButton(const char* label);
		bool OMNIFORCE_API InvisibleButton(const char* str_id, const ImVec2& size);
		
		// Input widgets with consistent styling
		bool OMNIFORCE_API InputText(const char* label, char* buffer, size_t buffer_size, ImGuiInputTextFlags flags = 0);
		bool OMNIFORCE_API InputTextWithHint(const char* label, const char* hint, char* buffer, size_t buffer_size, ImGuiInputTextFlags flags = 0);
		bool OMNIFORCE_API DragFloat(const char* label, float* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");
		bool OMNIFORCE_API DragFloat3(const char* label, float* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");
		bool OMNIFORCE_API Checkbox(const char* label, bool* value);
		
		// ============================================================================
		// Legacy Image Functions (keeping existing API)
		// ============================================================================
		
		void OMNIFORCE_API UnregisterImage(Ref<Image> image);
		void OMNIFORCE_API RenderImage(Ref<Image> image, Ref<ImageSampler> sampler, ImVec2 size, uint32 image_layer = 0, bool flip = false);
		bool OMNIFORCE_API RenderImageButton(Ref<Image> image, Ref<ImageSampler> sampler, ImVec2 size, uint32 image_layer = 0, bool flip = false);
	}
	
	// ============================================================================
	// Template Implementations
	// ============================================================================
	
	template<typename T>
	bool UI::PropertyFieldEnum(const char* label, T& value, const std::vector<std::pair<T, const char*>>& enum_values, const UIPropertyConfig& config) {
		// Find current value's display name
		const char* current_name = nullptr;
		for (const auto& pair : enum_values) {
			if (pair.first == value) {
				current_name = pair.second;
				break;
			}
		}
		
		if (!current_name) return false;
		
		// Create combo box
		if (ImGui::BeginCombo(label, current_name)) {
			for (const auto& pair : enum_values) {
				const bool is_selected = (pair.first == value);
				if (ImGui::Selectable(pair.second, is_selected)) {
					value = pair.first;
					if (config.on_changed) config.on_changed();
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		
		// Tooltip
		if (config.tooltip && ImGui::IsItemHovered()) {
			ImGui::SetTooltip(config.tooltip);
		}
		
		return true;
	}
}