#include "../Properties.h"

#include <Scene/Component.h>
#include <Scene/SceneRenderer.h>
#include <Asset/AssetManager.h>
#include <Core/Utils.h>
#include <Filesystem/Filesystem.h>
#include <Renderer/UI/ImGuiRenderer.h>
#include <DebugUtils/DebugRenderer.h>

#include "../../EditorUtils.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <tinyfiledialogs.h>

namespace Omni {

	void PropertiesPanel::Update()
	{
		if (m_IsOpen)
		{
			ImGui::Begin("Properties", &m_IsOpen);
			if (m_Selected) {
				std::string& tag = m_Entity.GetComponent<TagComponent>().tag;

				if(ImGui::BeginTable("properties_entity_name", 3, ImGuiTableFlags_SizingStretchProp))
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

						ImGui::BeginDisabled(m_Entity.HasComponent<MeshComponent>());
						if (ImGui::MenuItem("Mesh component")) {
							MeshComponent& mesh_component = m_Entity.AddComponent<MeshComponent>();
							mesh_component.mesh_handle = 0;
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<CameraComponent>());
						if (ImGui::MenuItem("Camera component")) {
							CameraComponent& camera_component = m_Entity.AddComponent<CameraComponent>();
							Ref<Camera3D> camera = CreateRef<Camera3D>(&g_PersistentAllocator);
							camera_component.camera = camera;
							camera_component.primary = false;
						}
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<RigidBodyComponent>());
						if (ImGui::MenuItem("Rigid body component")) {
							m_Entity.AddComponent<RigidBodyComponent>();
							if (m_Entity.GetParent().Valid())
								OMNIFORCE_CUSTOM_LOGGER_WARN("OmniEditor", "Added RigidBody component for game object \"{}\", which has parent. Rigid body is disabled if game object has parent.", tag.c_str());
						}
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<BoxColliderComponent>() || m_Entity.HasComponent<SphereColliderComponent>());
						if (ImGui::MenuItem("Box collider component"))
							m_Entity.AddComponent<BoxColliderComponent>();
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<SphereColliderComponent>() || m_Entity.HasComponent<BoxColliderComponent>());
						if (ImGui::MenuItem("Sphere collider component"))
							m_Entity.AddComponent<SphereColliderComponent>();
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<ScriptComponent>());
						if (ImGui::MenuItem("Script component"))
							m_Entity.AddComponent<ScriptComponent>();
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<PointLightComponent>());
						if (ImGui::MenuItem("Point light component"))
							m_Entity.AddComponent<PointLightComponent>();
						ImGui::EndDisabled();

						ImGui::EndPopup();
					}

					ImGui::EndTable();
				}

				auto& trs_component = m_Entity.GetComponent<TRSComponent>();

				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 5.0f });
				if(ImGui::BeginTable("TRS properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Translation");
					ImGui::SameLine();
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					ImGui::DragFloat3("##floatT", glm::value_ptr(trs_component.translation), 0.1f);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Rotation");
					ImGui::SameLine();
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

					glm::vec3 euler_angles = glm::eulerAngles(trs_component.rotation);
					euler_angles = glm::degrees(euler_angles);
					if (ImGui::DragFloat3("##floatR", glm::value_ptr(euler_angles), 0.1f))
						trs_component.rotation = glm::quat(glm::radians(euler_angles));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Scale");
					ImGui::SameLine();
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
					ImGui::DragFloat3("##floatS", glm::value_ptr(trs_component.scale), 0.01f);
					
					ImGui::EndTable();
				}
				ImGui::PopStyleVar();

				if (m_Entity.HasComponent<SpriteComponent>()) {
					if (ImGui::Button(" - ##sprite_component")) {
						m_Entity.RemoveComponent<SpriteComponent>();
					}
					else {
						ImGui::SameLine();
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

							if(ImGui::BeginTable("##sprite_component_properties_table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV));
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Material");

								ImGui::TableNextColumn();
								if (sc.texture) {
									AssetManager* am = AssetManager::Get();

									if (sc.texture) {
										Ref<Image> img = am->GetAsset<Image>(sc.texture);
										UI::RenderImage(img, m_Context->GetRenderer()->GetSamplerLinear(), { 50.0f, 50.0f / sc.aspect_ratio});
									}
									else {
										ImGui::Text("Drag texture here");
									}
								}
								else {
									ImGui::Text("Drag OFR texture here");
								}

								ImGui::EndTable();
							}
							ImGui::TreePop();
						}
					}
				}
				if (m_Entity.HasComponent<MeshComponent>()) {
					if (ImGui::Button(" - ##mesh_component")) {
						m_Entity.RemoveComponent<MeshComponent>();
					}
					else {
						ImGui::SameLine();
						if (ImGui::TreeNode("Mesh component")) {
							MeshComponent& mc = m_Entity.GetComponent<MeshComponent>();
							if (ImGui::BeginTable("##mesh_component_table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV));
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Level of Detail");

								ImGui::TableNextColumn();
								ImGui::Text("WIP"); //ImGui::SliderInt("##mesh_component_lod_slider", &mc.lod, 0, 3);

								ImGui::EndTable();
							}
							ImGui::TreePop();
						}
					}
				}
				if (m_Entity.HasComponent<CameraComponent>()) {
					if (ImGui::Button(" - ##camera_component")) {
						m_Entity.RemoveComponent<CameraComponent>();
					}
					else {
						ImGui::SameLine();
						if (ImGui::TreeNode("Camera component")) {

							CameraComponent& camera_component = m_Entity.GetComponent<CameraComponent>();
							CameraProjectionType projection_type = camera_component.camera->GetType();

							const char* items[] = { "Perspective", "Orthographic" };
							static int32 current_item = (int32)projection_type;

							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 5.0f });
							if(ImGui::BeginTable("Camera component properties table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
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
												Ref<Camera>& camera = camera_component.camera;
												Ref<Camera2D> camera_2D = CreateRef<Camera2D>(&g_PersistentAllocator);
												camera_2D->SetProjection(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f);

												camera_2D->SetType(CameraProjectionType::PROJECTION_2D);
												camera = camera_2D;
												break;
											}
											case (int32)CameraProjectionType::PROJECTION_3D: {
												Ref<Camera>& camera = camera_component.camera;
												Ref<Camera3D> camera_3D = CreateRef<Camera3D>(&g_PersistentAllocator);

												camera->SetType(CameraProjectionType::PROJECTION_3D);
												camera = camera_3D;
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
									Ref<Camera3D> camera_3D = camera_component.camera;

									ImGui::TableNextRow();

									ImGui::TableNextColumn();
									ImGui::Text("Field of view");

									ImGui::TableNextColumn();
									static float32 fov = camera_3D->GetFOV();
									if (ImGui::SliderAngle("##", &fov, 30.0f, 160.0f))
										camera_3D->SetFOV(fov);

								}
								else if (projection_type == CameraProjectionType::PROJECTION_2D) {
									Ref<Camera2D> camera_2D = camera_component.camera;

									ImGui::TableNextRow();

									ImGui::TableNextColumn();
									ImGui::Text("Scale");

									ImGui::TableNextColumn();
									static float32 orthographics_scale = camera_2D->GetScale();
									if (ImGui::DragFloat("##", &orthographics_scale, 0.01f, 0.01f, FLT_MAX))
										camera_2D->SetScale(orthographics_scale);
								}

								ImGui::TableNextRow();

								ImGui::TableNextColumn();
								ImGui::Text("Primary");

								ImGui::TableNextColumn();
								ImGui::Checkbox("##checkbox_camera_primary", &camera_component.primary);

								ImGui::EndTable();
							}
							ImGui::PopStyleVar();
							ImGui::TreePop();
						}
					}
				}
				if (m_Entity.HasComponent<RigidBodyComponent>()) {
					if (ImGui::Button(" - ##rb2d_component")) {
						m_Entity.RemoveComponent<RigidBodyComponent>();
					}
					else {
						ImGui::SameLine();
						if (ImGui::TreeNode("Rigid body component")) {
							RigidBodyComponent& rb2d_component = m_Entity.GetComponent<RigidBodyComponent>();

							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 5.0f });
							if(ImGui::BeginTable("##rb2d_properties_table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Motion type");
								ImGui::TableNextColumn();

								const char* motion_type_strings[] = { "Static", "Dynamic", "Kinematic" };
								if (ImGui::BeginCombo("##motion_type_combo", motion_type_strings[(int32)rb2d_component.type])) {
									for (int32 i = 0; i < IM_ARRAYSIZE(motion_type_strings); i++) {
										bool selected = i == (int32)rb2d_component.type;
										if (ImGui::Selectable(motion_type_strings[i], &selected))
											rb2d_component.type = (RigidBodyComponent::Type)i;
									}
									ImGui::EndCombo();
								}

								// Mass
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Mass (kg)");
								ImGui::TableNextColumn();
								ImGui::DragFloat("##rb2d_mass_property", &rb2d_component.mass, 0.1f, 0.001f, FLT_MAX);
								if (rb2d_component.mass < 0.0f) rb2d_component.mass = 0.001f;

								// Linear drag
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Linear drag");
								ImGui::TableNextColumn();
								ImGui::DragFloat("##rb2d_linear_drag_property", &rb2d_component.linear_drag, 0.01f, 0.0f, 1.0f);
								if (rb2d_component.linear_drag < 0.0f) rb2d_component.linear_drag = 0.0f;

								// Linear drag
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Angular drag");
								ImGui::TableNextColumn();
								ImGui::DragFloat("##rb2d_angular_drag_property", &rb2d_component.angular_drag, 0.01f, 0.0f, 1.0f);
								if (rb2d_component.angular_drag < 0.0f) rb2d_component.angular_drag = 0.0f;

								// Disable gravity
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Disable gravity");
								ImGui::TableNextColumn();
								ImGui::Checkbox("##rb2d_disable_gravity_property", &rb2d_component.disable_gravity);

								// Sensor mode
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Sensor mode");
								ImGui::TableNextColumn();
								ImGui::Checkbox("##rb2d_sensor_mode_property", &rb2d_component.sensor_mode);

								// lock Z axis
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Lock Z rotation");
								ImGui::TableNextColumn();
								ImGui::Checkbox("##rb2d_z_lock_property", &rb2d_component.lock_z_axis);

								ImGui::EndTable();
							}
							ImGui::PopStyleVar();
							ImGui::TreePop();
						}
					}
				}
				if (m_Entity.HasComponent<BoxColliderComponent>()) {
					if (ImGui::Button(" - ##box_collider_component")) {
						m_Entity.RemoveComponent<BoxColliderComponent>();
					}
					else {
						ImGui::SameLine();
						if (ImGui::TreeNode("Box collider component")) {
							BoxColliderComponent& box_collider_component = m_Entity.GetComponent<BoxColliderComponent>();
							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 5.0f });
							if(ImGui::BeginTable("##box_collider_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
							{
								// Size
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Size");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								if (ImGui::DragFloat3("##box_collider_size_property", (float32*)&box_collider_component.size, 0.01f, 0.01f, FLT_MAX)) {
									if (box_collider_component.size.x < 0.0f) box_collider_component.size.x = 0.01f;
									if (box_collider_component.size.y < 0.0f) box_collider_component.size.y = 0.01f;
									if (box_collider_component.size.z < 0.0f) box_collider_component.size.z = 0.01f;
								};
								
								if (ImGui::IsItemActive()) {
									TRSComponent trs_component = m_Entity.GetWorldTransform();
									// Multiply box collider size by 2 for rendering because Jolt takes half-size, and so in order to render it correctly we need to get full size (2x)
									DebugRenderer::RenderWireframeBox(trs_component.translation, trs_component.rotation, box_collider_component.size * 2.0f, { 0.28f, 0.27f, 1.0f });
								}

								// convex radius
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Convex radius");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								ImGui::SliderFloat("##box_collider_convex_radius_property", &box_collider_component.convex_radius, 0.0f, 25.0f);

								// Friction
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Friction");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								ImGui::SliderFloat("##box_collider_friction_property", &box_collider_component.friction, 0.0f, 1.0f);

								// Restitution
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Restitution");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								ImGui::SliderFloat("##box_collider_restitution_property", &box_collider_component.restitution, 0.0f, 1.0f);

								ImGui::EndTable();
							}
							ImGui::PopStyleVar();
							ImGui::TreePop();
						}
					}
				}
				if (m_Entity.HasComponent<SphereColliderComponent>()) {
					if (ImGui::Button(" - ##sphere_collder_component")) {
						m_Entity.RemoveComponent<SphereColliderComponent>();
					}
					else {
						ImGui::SameLine();
						if (ImGui::TreeNode("Sphere collider component")) {
							SphereColliderComponent& sphere_collider_component = m_Entity.GetComponent<SphereColliderComponent>();
							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 5.0f });
							if(ImGui::BeginTable("##sphere_collider_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
							{
								// Size
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Radius");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								if (ImGui::DragFloat("##sphere_collider_size_property", (float32*)&sphere_collider_component.radius, 0.01f, 0.01f, FLT_MAX))
									if (sphere_collider_component.radius < 0.0f) sphere_collider_component.radius = 0.01f;

								if (ImGui::IsItemActive()) {
									TRSComponent trs_component = m_Entity.GetWorldTransform();
									DebugRenderer::RenderWireframeSphere(trs_component.translation, sphere_collider_component.radius, { 0.28f, 0.27f, 1.0f });
								}

								// Friction
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Friction");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								ImGui::SliderFloat("##sphere_collider_friction_property", &sphere_collider_component.friction, 0.0f, 1.0f);

								// Restitution
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Restitution");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								ImGui::SliderFloat("##sphere_collider_restitution_property", &sphere_collider_component.restitution, 0.0f, 1.0f);
								ImGui::EndTable();
							}
							ImGui::PopStyleVar();
							ImGui::TreePop();
						}
					}
				}
				if (m_Entity.HasComponent<ScriptComponent>()) {
					if (ImGui::Button(" - ##script_component")) {
						m_Entity.RemoveComponent<ScriptComponent>();
					}
					else {
						ImGui::SameLine();
						if (ImGui::TreeNode("Script component")) {
							ScriptComponent& script_component = m_Entity.GetComponent<ScriptComponent>();
							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 5.0f });

							if (ImGui::BeginTable("##script_component_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH)) {
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Script name");

								ImGui::TableNextColumn();
								ImGui::InputText("##", script_component.class_name.data(), script_component.class_name.capacity());
								script_component.class_name.resize(strlen(script_component.class_name.c_str()));
								ImGui::EndTable();
							}

							ImGui::PopStyleVar();
							ImGui::TreePop();
						}
					}
				}
				if (m_Entity.HasComponent<PointLightComponent>()) {
					if (ImGui::Button(" - ##point_light_component")) {
						m_Entity.RemoveComponent<PointLightComponent>();
					}
					else {
						ImGui::SameLine();
						if (ImGui::TreeNode("Point light component")) {
							PointLightComponent& point_light_component = m_Entity.GetComponent<PointLightComponent>();
							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 20.0f, 10.0f });
							if (ImGui::BeginTable("##point_light_component_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
							{
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Intensity");
								ImGui::TableNextColumn();
								ImGui::DragFloat("##plc_props_intensity_drag", &point_light_component.intensity, 0.01f);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Min. radius");
								ImGui::TableNextColumn();
								ImGui::DragFloat("##plc_props_minrad_drag", &point_light_component.min_radius, 0.05f);
								if (ImGui::IsItemActive()) {
									DebugRenderer::RenderWireframeSphere(m_Entity.GetWorldTransform().translation, point_light_component.radius, { 0.28f, 0.27f, 1.0f });
									DebugRenderer::RenderWireframeSphere(m_Entity.GetWorldTransform().translation, point_light_component.min_radius, { 1.0f, 0.6f, 1.0f });
								}
								
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Max. radius");
								ImGui::TableNextColumn();
								ImGui::DragFloat("##plc_props_maxrad_drag", &point_light_component.radius, 0.05f);
								if (ImGui::IsItemActive()) {
									DebugRenderer::RenderWireframeSphere(m_Entity.GetWorldTransform().translation, point_light_component.radius, { 0.28f, 0.27f, 1.0f });
									DebugRenderer::RenderWireframeSphere(m_Entity.GetWorldTransform().translation, point_light_component.min_radius, { 1.0f, 0.6f, 1.0f });
								}

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::Text("Color");
								ImGui::SameLine();
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

								ImGui::ColorPicker4("", (float*)&point_light_component.color,
									ImGuiColorEditFlags_PickerHueWheel |
									ImGuiColorEditFlags_DisplayRGB | 
									ImGuiColorEditFlags_NoAlpha
								);

								ImGui::EndTable();
							}
							ImGui::PopStyleVar();
							ImGui::TreePop();
						}
					}
				}
			}
			ImGui::End();
		}
	}

	void PropertiesPanel::SetEntity(Entity entity, bool selected)
	{
		m_Entity = entity; m_Selected = selected;
	}

}