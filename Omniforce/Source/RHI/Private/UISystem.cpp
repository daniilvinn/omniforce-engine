#include <Foundation/Common.h>
#include <Rendering/UI/ImGuiRenderer.h>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace Omni {

	// ============================================================================
	// Global UI State
	// ============================================================================
	
	static bool s_UIInitialized = false;
	static UITheme s_CurrentTheme = UITheme::Dark;
	
	// Style stack tracking
	struct StyleStackEntry {
		int vars_pushed = 0;
		int colors_pushed = 0;
	};
	static std::vector<StyleStackEntry> s_StyleStack;
	
	// ============================================================================
	// Core UI System Implementation
	// ============================================================================
	
	namespace UI {
		
		void Initialize() {
			if (s_UIInitialized) return;
			
			// Don't automatically apply theme - preserve existing ImGui theme
			// Theme can be explicitly applied later if needed via ApplyTheme()
			s_CurrentTheme = UITheme::Dark; // Just track the current theme
			
			s_UIInitialized = true;
			OMNIFORCE_CORE_INFO("UI System initialized");
		}
		
		void Shutdown() {
			if (!s_UIInitialized) return;
			
			s_UIInitialized = false;
			OMNIFORCE_CORE_INFO("UI System shutdown");
		}
		
		void ApplyTheme(UITheme theme) {
			s_CurrentTheme = theme;
			
			ImGuiStyle& style = ImGui::GetStyle();
			ImGuiIO& io = ImGui::GetIO();
			
			switch (theme) {
				case UITheme::Dark: {
					ImGui::StyleColorsDark();

					// Dark theme and spacing - exact copy from VulkanImGuiRenderer.cpp
					style.Alpha = 1.0f;
					style.WindowPadding = ImVec2(8.0f, 6.0f);
					style.FramePadding = ImVec2(10.0f, 6.0f);
					style.ItemSpacing = ImVec2(8.0f, 6.0f);
					style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
					style.IndentSpacing = 18.0f;
					style.ScrollbarSize = 14.0f;
					style.GrabMinSize = 12.0f;
					style.WindowBorderSize = 1.0f;
					style.ChildBorderSize = 1.0f;
					style.PopupBorderSize = 1.0f;
					style.FrameBorderSize = 0.0f;
					style.TabBorderSize = 0.0f;
					style.WindowRounding = 4.0f;
					style.FrameRounding = 2.0f;
					style.GrabRounding = 2.0f;
					style.TabRounding = 3.0f;
					style.ScrollbarRounding = 2.0f;
					style.WindowMenuButtonPosition = ImGuiDir_None;

					// For multi-viewport, keep platform windows perfectly flat
					if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
					{
						style.WindowRounding = 0.0f;
						style.Colors[ImGuiCol_WindowBg].w = 1.0f;
					}

					// Core palette - exact copy from VulkanImGuiRenderer.cpp
					const ImVec4 accent = ImVec4(0.06f, 0.49f, 0.98f, 1.00f);
					style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.97f, 1.00f);
					style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
					style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
					style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
					style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.98f);
					style.Colors[ImGuiCol_Border] = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
					style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
					style.Colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
					style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
					style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
					style.Colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
					style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
					style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.06f, 0.60f);
					style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
					style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.04f, 0.05f, 0.54f);
					style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
					style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
					style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.33f, 0.33f, 0.36f, 1.00f);
					style.Colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.96f, 0.97f, 1.00f);
					style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.95f, 0.96f, 0.97f, 1.00f);
					style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.96f, 0.97f, 1.00f);
					style.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
					style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
					style.Colors[ImGuiCol_ButtonActive] = accent;
					style.Colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
					style.Colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.30f);
					style.Colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.80f);
					style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
					style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(accent.x, accent.y, accent.z, 0.78f);
					style.Colors[ImGuiCol_SeparatorActive] = ImVec4(accent.x, accent.y, accent.z, 1.00f);
					style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
					style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
					style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
					style.Colors[ImGuiCol_Tab] = ImVec4(0.09f, 0.09f, 0.10f, 0.95f);
					style.Colors[ImGuiCol_TabHovered] = ImVec4(accent.x, accent.y, accent.z, 0.80f);
					style.Colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
					style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
					style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
					style.Colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
					style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
					style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
					style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
					style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
					style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
					style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
					style.Colors[ImGuiCol_DragDropTarget] = accent;
					style.Colors[ImGuiCol_NavHighlight] = accent;
					style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
					style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
					style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
					
					break;
				}
					
				case UITheme::Light:
					ImGui::StyleColorsLight();
					break;
					
                    case UITheme::Custom:
					// Custom theme should be set via SetCustomTheme
					break;
                }
		}
		
		void SetCustomTheme(const ImGuiStyle& style) {
			ImGui::GetStyle() = style;
			s_CurrentTheme = UITheme::Custom;
		}
		
		// ============================================================================
		// Table System Implementation
		// ============================================================================
		
		Table::Table(const char* id, int columns, UITableStyle style)
			: m_Id(id), m_Columns(columns), m_Style(style), m_IsValid(false), m_StyleEntry{} {
			
			ApplyTableStyle();
			m_IsValid = ImGui::BeginTable(id, columns, GetTableFlags());
		}
		
		Table::~Table() {
			if (m_IsValid) {
				ImGui::EndTable();
			}
			
			// Pop any styles that were pushed in ApplyTableStyle()
			if (m_StyleEntry.vars_pushed > 0) {
				ImGui::PopStyleVar(m_StyleEntry.vars_pushed);
			}
			if (m_StyleEntry.colors_pushed > 0) {
				ImGui::PopStyleColor(m_StyleEntry.colors_pushed);
			}
		}
		
		void Table::SetupColumn(const char* label, ImGuiTableColumnFlags flags, float width) {
			if (!m_IsValid) return;
			ImGui::TableSetupColumn(label, flags, width);
		}
		
		void Table::SetupColumn(const char* label, float width_ratio) {
			if (!m_IsValid) return;
			ImGui::TableSetupColumn(label, ImGuiTableColumnFlags_WidthStretch, 0.0f);
		}
		
		void Table::SetupColumns(const std::vector<UITableColumn>& columns) {
			if (!m_IsValid) return;
			
			for (const auto& col : columns) {
				if (col.width_ratio > 0.0f) {
					ImGui::TableSetupColumn(col.label.c_str(), ImGuiTableColumnFlags_WidthStretch, 0.0f);
				} else {
					ImGui::TableSetupColumn(col.label.c_str(), col.flags, col.width);
				}
			}
		}
		
		void Table::NextRow() {
			if (!m_IsValid) return;
			ImGui::TableNextRow();
		}
		
		void Table::NextColumn() {
			if (!m_IsValid) return;
			ImGui::TableNextColumn();
		}
		
		void Table::SetColumnIndex(int column) {
			if (!m_IsValid) return;
			ImGui::TableSetColumnIndex(column);
		}
		
		void Table::CellText(const char* text) {
			if (!m_IsValid) return;
			ImGui::Text("%s", text);
		}
		
		void Table::CellTextFmt(const char* fmt, ...) {
			if (!m_IsValid) return;
			va_list args;
			va_start(args, fmt);
			ImGui::TextV(fmt, args);
			va_end(args);
		}
		
		void Table::CellTextColored(const ImVec4& color, const char* text) {
			if (!m_IsValid) return;
			ImGui::TextColored(color, "%s", text);
		}
		
		void Table::CellButton(const char* label, const ImVec2& size) {
			if (!m_IsValid) return;
			ImGui::Button(label, size);
		}
		
		void Table::CellCheckbox(const char* label, bool* value) {
			if (!m_IsValid) return;
			ImGui::Checkbox(label, value);
		}
		
		void Table::CellDragFloat(const char* label, float* value, float speed, float min, float max, const char* format) {
			if (!m_IsValid) return;
			ImGui::DragFloat(label, value, speed, min, max, format);
		}
		
		void Table::CellDragFloat3(const char* label, float* value, float speed, float min, float max, const char* format) {
			if (!m_IsValid) return;
			ImGui::DragFloat3(label, value, speed, min, max, format);
		}
		
		void Table::CellInputText(const char* label, char* buffer, size_t buffer_size, ImGuiInputTextFlags flags) {
			if (!m_IsValid) return;
			ImGui::InputText(label, buffer, buffer_size, flags);
		}
		
		void Table::HeadersRow() {
			if (!m_IsValid) return;
			ImGui::TableHeadersRow();
		}
		
		void Table::ApplyTableStyle() {
			// Reset style entry tracking
			m_StyleEntry = {};
			
			switch (m_Style) {
				case UITableStyle::Default:
					// Default style - no special styling needed
					break;
					
				case UITableStyle::Compact:
					ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, 3.0f));
					m_StyleEntry.vars_pushed = 1;
					break;
					
				case UITableStyle::Properties:
					ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 6.0f));
					m_StyleEntry.vars_pushed = 1;
					break;
					
				case UITableStyle::DataGrid:
					// Data grid style - full featured
					break;
					
				case UITableStyle::Logs:
					ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.13f, 0.14f, 0.15f, 1.00f));
					ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.19f, 0.20f, 0.21f, 1.00f));
					m_StyleEntry.colors_pushed = 2;
					break;
			}
		}
		
		ImGuiTableFlags Table::GetTableFlags() const {
			ImGuiTableFlags flags = ImGuiTableFlags_None;
			
			switch (m_Style) {
				case UITableStyle::Default:
					flags |= ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg;
					break;
					
				case UITableStyle::Compact:
					flags |= ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV;
					break;
					
				case UITableStyle::Properties:
					flags |= ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV;
					break;
					
				case UITableStyle::DataGrid:
					flags |= ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
							ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
							ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
					break;
					
				case UITableStyle::Logs:
					flags |= ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg;
					break;
			}
			
			return flags;
		}
		
		// ============================================================================
		// Convenience Table Functions
		// ============================================================================
		
		bool BeginPropertyTable(const char* id, bool* open) {
			PushTableStyle(UITableStyle::Properties);
			return ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV);
		}
		
		void EndPropertyTable() {
			ImGui::EndTable();
			PopTableStyle();
		}
		
		bool BeginDataTable(const char* id, const std::vector<UITableColumn>& columns, bool* open) {
			PushTableStyle(UITableStyle::DataGrid);
			
			ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
								   ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
								   ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
			
			bool result = ImGui::BeginTable(id, (int)columns.size(), flags);
			
			if (result) {
				for (const auto& col : columns) {
					if (col.width_ratio > 0.0f) {
						ImGui::TableSetupColumn(col.label.c_str(), ImGuiTableColumnFlags_WidthStretch, 0.0f);
					} else {
						ImGui::TableSetupColumn(col.label.c_str(), col.flags, col.width);
					}
				}
				ImGui::TableHeadersRow();
			}
			
			return result;
		}
		
		void EndDataTable() {
			ImGui::EndTable();
			PopTableStyle();
		}
		
		bool BeginLogTable(const char* id, bool* open) {
			PushTableStyle(UITableStyle::Logs);
			return ImGui::BeginTable(id, 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg);
		}
		
		void EndLogTable() {
			ImGui::EndTable();
			PopTableStyle();
		}
		
		// ============================================================================
		// Property Field System Implementation
		// ============================================================================
		
		bool PropertyField(const char* label, float& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text(config.format ? config.format : "%.3f", value);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::DragFloat(("##" + std::string(label)).c_str(), &value, config.speed, config.min_value, config.max_value, config.format);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyField(const char* label, glm::vec3& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text(config.format ? config.format : "%.3f, %.3f, %.3f", value.x, value.y, value.z);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::DragFloat3((std::string(label)).c_str(), &value.x, config.speed, config.min_value, config.max_value, config.format);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyField(const char* label, bool& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text("%s", value ? "True" : "False");
			} else {
				changed = ImGui::Checkbox(("##" + std::string(label)).c_str(), &value);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyField(const char* label, std::string& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text("%s", value.c_str());
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				// Use a buffer for input text
				static char buffer[256];
				strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
				buffer[sizeof(buffer) - 1] = '\0';
				
				if (ImGui::InputText(("##" + std::string(label)).c_str(), buffer, sizeof(buffer))) {
					value = std::string(buffer);
					changed = true;
				}
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		// ============================================================================
		// Panel System Implementation
		// ============================================================================
		
		Panel::Panel(const char* name, bool* open, ImGuiWindowFlags flags)
			: m_Name(name), m_OpenPtr(open), m_IsOpen(false), m_Flags(flags) {
			m_IsOpen = ImGui::Begin(name, open, flags);
		}
		
		Panel::~Panel() {
			if (m_IsOpen) {
				ImGui::End();
			}
		}
		
		bool BeginPanel(const char* name, bool* open, ImGuiWindowFlags flags) {
			return ImGui::Begin(name, open, flags);
		}
		
		void EndPanel() {
			ImGui::End();
		}
		
		// ============================================================================
		// Style Management Implementation
		// ============================================================================
		
		void PushCompactStyle() {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 3.0f));
		}
		
		void PopCompactStyle() {
			ImGui::PopStyleVar(2);
		}
		
		void PushPanelStyle() {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
		}
		
		void PopPanelStyle() {
			ImGui::PopStyleVar(1);
		}
		
		void PushTableStyle(UITableStyle style) {
			StyleStackEntry entry = {};
			
			switch (style) {
				case UITableStyle::Compact:
					ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, 3.0f));
					entry.vars_pushed = 1;
					break;
				case UITableStyle::Properties:
					ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 6.0f));
					entry.vars_pushed = 1;
					break;
				case UITableStyle::Logs:
					ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.13f, 0.14f, 0.15f, 1.00f));
					ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.19f, 0.20f, 0.21f, 1.00f));
					entry.colors_pushed = 2;
					break;
				default:
					break;
			}
			
			s_StyleStack.push_back(entry);
		}
		
		void PopTableStyle() {
			if (s_StyleStack.empty()) return;
			
			StyleStackEntry entry = s_StyleStack.back();
			s_StyleStack.pop_back();
			
			if (entry.vars_pushed > 0) {
				ImGui::PopStyleVar(entry.vars_pushed);
			}
			if (entry.colors_pushed > 0) {
				ImGui::PopStyleColor(entry.colors_pushed);
			}
		}
		
		// ============================================================================
		// Utility Functions Implementation
		// ============================================================================
		
		void Text(const char* text) {
			ImGui::Text("%s", text);
		}
		
		void TextColored(const ImVec4& color, const char* text) {
			ImGui::TextColored(color, "%s", text);
		}
		
		void TextDisabled(const char* text) {
			ImGui::TextDisabled("%s", text);
		}
		
		void TextWrapped(const char* text) {
			ImGui::TextWrapped("%s", text);
		}
		
		bool Button(const char* label, const ImVec2& size) {
			return ImGui::Button(label, size);
		}
		
		bool SmallButton(const char* label) {
			return ImGui::SmallButton(label);
		}
		
		bool InvisibleButton(const char* str_id, const ImVec2& size) {
			return ImGui::InvisibleButton(str_id, size);
		}
		
		bool InputText(const char* label, char* buffer, size_t buffer_size, ImGuiInputTextFlags flags) {
			return ImGui::InputText(label, buffer, buffer_size, flags);
		}
		
		bool InputTextWithHint(const char* label, const char* hint, char* buffer, size_t buffer_size, ImGuiInputTextFlags flags) {
			return ImGui::InputTextWithHint(label, hint, buffer, buffer_size, flags);
		}
		
		bool DragFloat(const char* label, float* value, float speed, float min, float max, const char* format) {
			return ImGui::DragFloat(label, value, speed, min, max, format);
		}
		
		bool DragFloat3(const char* label, float* value, float speed, float min, float max, const char* format) {
			return ImGui::DragFloat3(label, value, speed, min, max, format);
		}
		
		bool Checkbox(const char* label, bool* value) {
			return ImGui::Checkbox(label, value);
		}
		
		// ============================================================================
		// Additional Property Field Overloads
		// ============================================================================
		
		bool PropertyField(const char* label, glm::vec2& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text(config.format ? config.format : "%.3f, %.3f", value.x, value.y);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::DragFloat2(("##" + std::string(label)).c_str(), &value.x, config.speed, config.min_value, config.max_value, config.format);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyField(const char* label, glm::vec4& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text(config.format ? config.format : "%.3f, %.3f, %.3f, %.3f", value.x, value.y, value.z, value.w);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::DragFloat4(("##" + std::string(label)).c_str(), &value.x, config.speed, config.min_value, config.max_value, config.format);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyField(const char* label, int& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text("%d", value);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::DragInt(("##" + std::string(label)).c_str(), &value, config.speed, (int)config.min_value, (int)config.max_value, config.format);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyField(const char* label, ImVec4& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text("%.3f, %.3f, %.3f, %.3f", value.x, value.y, value.z, value.w);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::ColorEdit4(("##" + std::string(label)).c_str(), (float*)&value, ImGuiColorEditFlags_NoInputs);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyFieldAsset(const char* label, std::string& asset_path, const char* file_filter, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text("%s", asset_path.c_str());
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				// Use a buffer for input text
				static char buffer[256];
				strncpy(buffer, asset_path.c_str(), sizeof(buffer) - 1);
				buffer[sizeof(buffer) - 1] = '\0';
				
				if (ImGui::InputText(("##" + std::string(label)).c_str(), buffer, sizeof(buffer))) {
					asset_path = std::string(buffer);
					changed = true;
				}
				
				// TODO: Add drag & drop support for assets
				// TODO: Add file picker button
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		// ============================================================================
		// Additional Property Field Overloads for glm::ivec types
		// ============================================================================
		
		bool PropertyField(const char* label, glm::ivec2& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text("%d, %d", value.x, value.y);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::DragInt2(("##" + std::string(label)).c_str(), &value.x, config.speed, (int)config.min_value, (int)config.max_value, config.format);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyField(const char* label, glm::ivec3& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text("%d, %d, %d", value.x, value.y, value.z);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::DragInt3(("##" + std::string(label)).c_str(), &value.x, config.speed, (int)config.min_value, (int)config.max_value, config.format);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
		bool PropertyField(const char* label, glm::ivec4& value, const UIPropertyConfig& config) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine();
			
			bool changed = false;
			if (config.read_only) {
				ImGui::Text("%d, %d, %d, %d", value.x, value.y, value.z, value.w);
			} else {
				ImGui::SetNextItemWidth(-1.0f);
				changed = ImGui::DragInt4(("##" + std::string(label)).c_str(), &value.x, config.speed, (int)config.min_value, (int)config.max_value, config.format);
			}
			
			if (config.tooltip && ImGui::IsItemHovered()) {
				ImGui::SetTooltip(config.tooltip);
			}
			
			if (changed && config.on_changed) {
				config.on_changed();
			}
			
			return changed;
		}
		
	} // namespace UI
	
} // namespace Omni
