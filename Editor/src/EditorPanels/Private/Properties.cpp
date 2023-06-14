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
							SpriteComponent& sprite_component = m_Entity.AddComponent<SpriteComponent>();
							sprite_component.texture = m_Context->GetRenderer()->GetDummyWhiteTexture();
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<CameraComponent>());
						if (ImGui::MenuItem("Camera component")) {
							CameraComponent& camera_component = m_Entity.AddComponent<CameraComponent>();
							Shared<Camera3D> camera = std::make_shared<Camera3D>();
							camera->SetProjection(glm::radians(80.0f), 16.0 / 9.0f, 0.1f, 100.0f);
							camera_component.camera = camera;
							camera_component.primary = false;
						}
						ImGui::EndDisabled();

						ImGui::EndPopup();
					}
				}
				ImGui::EndTable();

				glm::mat4& transform = m_Entity.GetComponent<TransformComponent>().matrix;
				auto& trs_component = m_Entity.GetComponent<TRSComponent>();

				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 5.0f });
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
				ImGui::PopStyleVar();

				if (m_Entity.HasComponent<SpriteComponent>()) {
					if (ImGui::TreeNode("Sprite component")) {
						SpriteComponent& sc = m_Entity.GetComponent<SpriteComponent>();

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
									sc.aspect_ratio = { (float32)texture_resolution.x / (float32)texture_resolution.y };
								}
							};
						}
						ImGui::EndTable();
						ImGui::TreePop();
					}
				}
				if (m_Entity.HasComponent<CameraComponent>()) {
					if (ImGui::TreeNode("Camera component")) {
						CameraComponent& camera_component = m_Entity.GetComponent<CameraComponent>();
						CameraProjectionType projection_type = camera_component.camera->GetType();

						const char* items[] = { "Perspective", "Orthographic" };
						static int32 current_item = (int32)projection_type;

						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 5.0f });
						ImGui::BeginTable("Camera component properties table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH);
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("Type");

							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
							if (ImGui::BeginCombo("##combo", items[(int32)projection_type]))
							{
								for (int n = 0; n < IM_ARRAYSIZE(items); n++)
								{
									bool is_selected = (current_item == n);
									if (ImGui::Selectable(items[n], is_selected, 0)) {
										current_item = n;

										switch (current_item)
										{
										case (int32)CameraProjectionType::PROJECTION_2D: {
											Shared<Camera>& camera = camera_component.camera;
											Shared<Camera2D> camera_2D = std::make_shared<Camera2D>();
											camera_2D->SetProjection(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f);

											camera_2D->SetType(CameraProjectionType::PROJECTION_2D);
											camera = ShareAs<Camera>(camera_2D);
											break;
										}
										case (int32)CameraProjectionType::PROJECTION_3D: {
											Shared<Camera>& camera = camera_component.camera;
											Shared<Camera3D> camera_3D = std::make_shared<Camera3D>();
											camera_3D->SetProjection(glm::radians(90.0f), 16.0 / 9.0, 0.0f, 100.0f);
											camera_3D->Move({ 0.0f, 0.0f, -50.0f });

											camera->SetType(CameraProjectionType::PROJECTION_3D);
											camera = ShareAs<Camera>(camera_3D);
											break;
										}

										default:
											break;
										}

										if (is_selected)
											ImGui::SetItemDefaultFocus();

									}
								}
								ImGui::EndCombo();
							}

							if (projection_type == CameraProjectionType::PROJECTION_3D) {
								Shared<Camera3D> camera_3D = ShareAs<Camera3D>(camera_component.camera);

								ImGui::TableNextRow();

								ImGui::TableNextColumn();
								ImGui::Text("Field of view");

								ImGui::TableNextColumn();
								static float32 fov = camera_3D->GetFOV();
								if (ImGui::SliderAngle("##", &fov, 30.0f, 160.0f))
									camera_3D->SetFOV(fov);

							}
							else if (projection_type == CameraProjectionType::PROJECTION_2D) {
								Shared<Camera2D> camera_2D = ShareAs<Camera2D>(camera_component.camera);

								ImGui::TableNextRow();

								ImGui::TableNextColumn();
								ImGui::Text("Scale");

								ImGui::TableNextColumn();
								static float32 orthographics_scale = camera_2D->GetScale();
								if (ImGui::DragFloat("##", &orthographics_scale, 0.01f, 0.01f, FLT_MAX))
									camera_2D->SetScale(orthographics_scale);
							}
						}

						ImGui::TableNextRow();

						ImGui::TableNextColumn();
						ImGui::Text("Primary");

						ImGui::TableNextColumn();
						ImGui::Checkbox("##checkbox_camera_primary", &camera_component.primary);

						ImGui::EndTable();
						ImGui::PopStyleVar();
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