#include "../Properties.h"

#include <Scene/Component.h>
#include <Scene/SceneRenderer.h>
#include <Asset/AssetManager.h>

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <tinyfiledialogs/tinyfiledialogs.h>

namespace Omni {

	void PropertiesPanel::Render()
	{
		ImGui::Begin("Properties", &m_IsOpen);
		if (m_IsOpen)
		{
			if (m_Selected) {
				std::string& tag = m_Entity.GetComponent<TagComponent>().tag;

				ImGui::BeginTable("properties_entity_name", 3, ImGuiTableFlags_SizingStretchProp);
				{
					ImGui::TableNextColumn();
					ImGui::Text("Entity name: ");

					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					ImGui::InputText("##", tag.data(), tag.capacity());
					tag.resize(strlen(tag.c_str()));

					ImGui::TableNextColumn();
					if (ImGui::Button(" + ", { 0.0f, 0.0f })) {
						ImGui::OpenPopup("Components popup");
					}
					
					if (ImGui::BeginPopup("Components popup")) {
						ImGui::BeginDisabled(m_Entity.HasComponent<SpriteComponent>());
						if (ImGui::MenuItem("Sprite component")) {
							m_Entity.AddComponent<SpriteComponent>();
							SpriteComponent& sc = m_Entity.GetComponent<SpriteComponent>();
							sc.texture = m_Context->GetRenderer()->GetDummyWhiteTexture();
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();
						ImGui::EndPopup();
					}
				}
				ImGui::EndTable();

				glm::mat4& transform = m_Entity.GetComponent<TransformComponent>().matrix;
				auto& trs_component = m_Entity.GetComponent<TRSComponent>();

				ImGui::BeginTable("Properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH);
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Translation");
					ImGui::SameLine();
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat3("##floatT", glm::value_ptr(trs_component.translation), 0.1f))
						RecalculateTransform();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Rotation");
					ImGui::SameLine();
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat3("##floatR", glm::value_ptr(trs_component.rotation), 0.1f))
						RecalculateTransform();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Scale");
					ImGui::SameLine();
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					if (ImGui::DragFloat3("##floatS", glm::value_ptr(trs_component.scale), 0.01f))
						RecalculateTransform();
				}
				ImGui::EndTable();

				if (m_Entity.HasComponent<SpriteComponent>()) {
					SpriteComponent& sc = m_Entity.GetComponent<SpriteComponent>();
					if (ImGui::TreeNode("Sprite component")) {

						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
						ImGui::Text("Layer");
						ImGui::SameLine();
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5);
						ImGui::DragInt("##", &sc.layer, 0.05, 0, INT32_MAX);
						ImGui::Separator();

						ImGui::ColorPicker4("Color tint", (float*)&sc.color, 
							ImGuiColorEditFlags_PickerHueWheel |
							ImGuiColorEditFlags_AlphaBar |
							ImGuiColorEditFlags_DisplayRGB
						);

						ImGui::Separator();

						ImGui::BeginTable("properties_entity_name", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV);
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("Material");

							ImGui::TableNextColumn();
							if (ImGui::Button("Browse files", {-FLT_MIN, 0.0f})) {
								const char* filters[] = { "*.png", "*.jpg", "*.jpeg" };

								char* filepath = tinyfd_openFileDialog(
									"Open file",
									(std::filesystem::current_path().string() + "\\assets\\textures\\").c_str(),
									3,
									filters,
									NULL,
									false
								);

								if (filepath != NULL) {
									sc.texture = AssetManager::Get()->LoadTexture(std::filesystem::path(filepath));
									m_Context->GetRenderer()->AcquireTextureIndex(AssetManager::Get()->GetTexture(sc.texture), SamplerFilteringMode::NEAREST);
									uvec3 texture_resolution = AssetManager::Get()->GetTexture(sc.texture)->GetSpecification().extent;
									TRSComponent& trs = m_Entity.GetComponent<TRSComponent>();
									trs.scale.x = { (float32)texture_resolution.x / (float32)texture_resolution.y };
								}
							};
						}
						ImGui::EndTable();
						ImGui::TreePop();
					}
				}				
			}
		}
		ImGui::End();
	}

	void PropertiesPanel::SetEntity(Entity entity, bool selected)
	{
		m_Entity = entity; m_Selected = selected;
	}

	void PropertiesPanel::RecalculateTransform()
	{
		glm::mat4& matrix = m_Entity.GetComponent<TransformComponent>().matrix;
		auto& trs_component = m_Entity.GetComponent<TRSComponent>();

		matrix = glm::translate(glm::mat4(1.0f), trs_component.translation) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
			glm::scale(glm::mat4(1.0f), trs_component.scale);

	}

}