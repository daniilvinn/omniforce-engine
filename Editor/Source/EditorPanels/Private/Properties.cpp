#include "../Properties.h"

#include <Scene/Component.h>
#include <Rendering/ISceneRenderer.h>
#include <Rendering/UI/ImGuiRenderer.h>
#include <Asset/AssetManager.h>
#include <Core/Utils.h>
#include <Filesystem/Filesystem.h>
#include <DebugUtils/DebugRenderer.h>

// #include "../../EditorUtils.h" // Not used directly in this file

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

				// Remove window padding for full-width tables
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
				
				if(ImGui::BeginTable("properties_entity_header", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
				{
					ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
					
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Name");

					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-1.0f);
					ImGui::InputText("##entity_name", tag.data(), tag.capacity());
					tag.resize(strlen(tag.c_str()));

					ImGui::EndTable();
				}
				ImGui::PopStyleVar(2);

				// Restore padding for other elements
				ImGui::Spacing();
				
				// Add Component button
				if (ImGui::Button("Add Component", { -1.0f, 0.0f })) {
					ImGui::OpenPopup("Add Component");
				}

					if (ImGui::BeginPopup("Add Component")) {
						// Rendering Components
						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Rendering");
						ImGui::Separator();

						ImGui::BeginDisabled(m_Entity.HasComponent<SpriteComponent>());
						if (ImGui::MenuItem("Sprite Renderer")) {
							SpriteComponent& sprite_component = m_Entity.AddComponent<SpriteComponent>();
							sprite_component.texture = m_Context->GetRenderer()->GetDummyWhiteTexture();
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<MeshComponent>());
						if (ImGui::MenuItem("Mesh Renderer")) {
							MeshComponent& mesh_component = m_Entity.AddComponent<MeshComponent>();
							mesh_component.mesh_handle = 0;
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::Spacing();

						// Camera
						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Camera");
						ImGui::Separator();

						ImGui::BeginDisabled(m_Entity.HasComponent<CameraComponent>());
						if (ImGui::MenuItem("Camera")) {
							CameraComponent& camera_component = m_Entity.AddComponent<CameraComponent>();
							Ref<Camera3D> camera = CreateRef<Camera3D>(&g_PersistentAllocator);
							camera_component.camera = camera;
							camera_component.primary = false;
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::Spacing();

						// Physics Components
						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Physics");
						ImGui::Separator();

						ImGui::BeginDisabled(m_Entity.HasComponent<RigidBodyComponent>());
						if (ImGui::MenuItem("Rigidbody")) {
							m_Entity.AddComponent<RigidBodyComponent>();
							if (m_Entity.GetParent().Valid())
								OMNIFORCE_CUSTOM_LOGGER_WARN("OmniEditor", "Added RigidBody component for game object \"{}\", which has parent. Rigid body is disabled if game object has parent.", tag.c_str());
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<BoxColliderComponent>() || m_Entity.HasComponent<SphereColliderComponent>());
						if (ImGui::MenuItem("Box Collider")) {
							m_Entity.AddComponent<BoxColliderComponent>();
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::BeginDisabled(m_Entity.HasComponent<SphereColliderComponent>() || m_Entity.HasComponent<BoxColliderComponent>());
						if (ImGui::MenuItem("Sphere Collider")) {
							m_Entity.AddComponent<SphereColliderComponent>();
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::Spacing();

						// Lighting Components
						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Lighting");
						ImGui::Separator();

						ImGui::BeginDisabled(m_Entity.HasComponent<PointLightComponent>());
						if (ImGui::MenuItem("Point Light")) {
							m_Entity.AddComponent<PointLightComponent>();
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::Spacing();

						// Scripting Components
						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Scripting");
						ImGui::Separator();

						ImGui::BeginDisabled(m_Entity.HasComponent<ScriptComponent>());
						if (ImGui::MenuItem("Script")) {
							m_Entity.AddComponent<ScriptComponent>();
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndDisabled();

						ImGui::EndPopup();
					}

				
				// Transform Component Header
				ImGui::Separator();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
				ImGui::Text("Transform");
				ImGui::PopStyleColor();

				auto& trs_component = m_Entity.GetComponent<TRSComponent>();

				// Remove window padding for transform table
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 6.0f));
				if(ImGui::BeginTable("TRS properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
				{
					ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
					
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Position");
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-1.0f);
					ImGui::DragFloat3("##position", glm::value_ptr(trs_component.translation), 0.1f, -FLT_MAX, FLT_MAX, "%.2f");

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Rotation");
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-1.0f);

					glm::vec3 euler_angles = glm::eulerAngles(trs_component.rotation);
					euler_angles = glm::degrees(euler_angles);
					if (ImGui::DragFloat3("##rotation", glm::value_ptr(euler_angles), 1.0f, -180.0f, 180.0f, "%.1f°"))
						trs_component.rotation = glm::quat(glm::radians(euler_angles));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Scale");
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-1.0f);
					ImGui::DragFloat3("##scale", glm::value_ptr(trs_component.scale), 0.01f, 0.01f, FLT_MAX, "%.2f");
					
					ImGui::EndTable();
				}
				ImGui::PopStyleVar(2);

				if (m_Entity.HasComponent<SpriteComponent>()) {
					ImGui::Spacing();
					ImGui::Separator();
					
					// Component header
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
					bool sprite_open = ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleColor();
					
					ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
					if (ImGui::SmallButton(" - ##sprite_component")) {
						m_Entity.RemoveComponent<SpriteComponent>();
					}
					
					if (sprite_open) {
						SpriteComponent& sc = m_Entity.GetComponent<SpriteComponent>();

						// Remove window padding for component table
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
						
						if(ImGui::BeginTable("##sprite_component_table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
						{
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Layer");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::DragInt("##sprite_layer", &sc.layer, 0.1f, 0, INT32_MAX);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Color");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::ColorEdit4("##sprite_color", (float*)&sc.color, ImGuiColorEditFlags_NoInputs);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Texture");
							ImGui::TableNextColumn();
							if (sc.texture) {
								AssetManager* am = AssetManager::Get();
								if (sc.texture) {
									Ref<Image> img = am->GetAsset<Image>(sc.texture);
									UI::RenderImage(img, m_Context->GetRenderer()->GetSamplerLinear(), { 50.0f, 50.0f / sc.aspect_ratio});
								}
							}
							else {
								ImGui::Text("Drag OFR texture here");
							}

							ImGui::EndTable();
						}
						ImGui::PopStyleVar(2);
					}
				}
				if (m_Entity.HasComponent<MeshComponent>()) {
					ImGui::Spacing();
					ImGui::Separator();
					
					// Component header
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
					bool mesh_open = ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleColor();
					
					ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
					if (ImGui::SmallButton(" - ##mesh_component")) {
						m_Entity.RemoveComponent<MeshComponent>();
					}
					
					if (mesh_open) {
						MeshComponent& mc = m_Entity.GetComponent<MeshComponent>();

						// Remove window padding for component table
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
						
						if (ImGui::BeginTable("##mesh_component_table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
						{
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Mesh");
							ImGui::TableNextColumn();
							ImGui::Text("WIP - Drag mesh asset here");

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("LOD");
							ImGui::TableNextColumn();
							ImGui::Text("WIP"); //ImGui::SliderInt("##mesh_component_lod_slider", &mc.lod, 0, 3);

							ImGui::EndTable();
						}
						ImGui::PopStyleVar(2);
					}
				}
				if (m_Entity.HasComponent<CameraComponent>()) {
					ImGui::Spacing();
					ImGui::Separator();
					
					// Component header
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
					bool camera_open = ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleColor();
					
					ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
					if (ImGui::SmallButton(" - ##camera_component")) {
						m_Entity.RemoveComponent<CameraComponent>();
					}
					
					if (camera_open) {
						CameraComponent& camera_component = m_Entity.GetComponent<CameraComponent>();
						CameraProjectionType projection_type = camera_component.camera->GetType();

						const char* items[] = { "Perspective", "Orthographic" };
						static int32 current_item = (int32)projection_type;

						// Remove window padding for component table
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
						
						if(ImGui::BeginTable("##camera_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
						{
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Type");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							if (ImGui::BeginCombo("##camera_type", items[(int32)projection_type]))
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
								ImGui::AlignTextToFramePadding();
								ImGui::Text("FOV");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(-1.0f);
								static float32 fov = camera_3D->GetFOV();
								if (ImGui::SliderAngle("##camera_fov", &fov, 30.0f, 160.0f))
									camera_3D->SetFOV(fov);
							}
							else if (projection_type == CameraProjectionType::PROJECTION_2D) {
								Ref<Camera2D> camera_2D = camera_component.camera;

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::AlignTextToFramePadding();
								ImGui::Text("Scale");
								ImGui::TableNextColumn();
								ImGui::SetNextItemWidth(-1.0f);
								static float32 orthographics_scale = camera_2D->GetScale();
								if (ImGui::DragFloat("##camera_scale", &orthographics_scale, 0.01f, 0.01f, FLT_MAX))
									camera_2D->SetScale(orthographics_scale);
							}

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Primary");
							ImGui::TableNextColumn();
							ImGui::Checkbox("##camera_primary", &camera_component.primary);

							ImGui::EndTable();
						}
						ImGui::PopStyleVar(2);
					}
				}
				if (m_Entity.HasComponent<RigidBodyComponent>()) {
					ImGui::Spacing();
					ImGui::Separator();
					
					// Component header
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
					bool rigidbody_open = ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleColor();
					
					ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
					if (ImGui::SmallButton(" - ##rb2d_component")) {
						m_Entity.RemoveComponent<RigidBodyComponent>();
					}
					
					if (rigidbody_open) {
						RigidBodyComponent& rb2d_component = m_Entity.GetComponent<RigidBodyComponent>();

						// Remove window padding for component table
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
						
						if(ImGui::BeginTable("##rigidbody_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
						{
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Type");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							const char* motion_type_strings[] = { "Static", "Dynamic", "Kinematic" };
							if (ImGui::BeginCombo("##motion_type", motion_type_strings[(int32)rb2d_component.type])) {
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
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Mass");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::DragFloat("##rb_mass", &rb2d_component.mass, 0.1f, 0.001f, FLT_MAX, "%.3f kg");
							if (rb2d_component.mass < 0.0f) rb2d_component.mass = 0.001f;

							// Linear drag
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Linear Drag");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::DragFloat("##rb_linear_drag", &rb2d_component.linear_drag, 0.01f, 0.0f, 1.0f, "%.2f");
							if (rb2d_component.linear_drag < 0.0f) rb2d_component.linear_drag = 0.0f;

							// Angular drag
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Angular Drag");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::DragFloat("##rb_angular_drag", &rb2d_component.angular_drag, 0.01f, 0.0f, 1.0f, "%.2f");
							if (rb2d_component.angular_drag < 0.0f) rb2d_component.angular_drag = 0.0f;

							// Disable gravity
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Gravity");
							ImGui::TableNextColumn();
							bool use_gravity = !rb2d_component.disable_gravity;
							if (ImGui::Checkbox("##rb_use_gravity", &use_gravity))
								rb2d_component.disable_gravity = !use_gravity;

							// Sensor mode
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Is Trigger");
							ImGui::TableNextColumn();
							ImGui::Checkbox("##rb_sensor", &rb2d_component.sensor_mode);

							// lock Z axis
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Lock Z Axis");
							ImGui::TableNextColumn();
							ImGui::Checkbox("##rb_lock_z", &rb2d_component.lock_z_axis);

							ImGui::EndTable();
						}
						ImGui::PopStyleVar(2);
					}
				}
				if (m_Entity.HasComponent<BoxColliderComponent>()) {
					ImGui::Spacing();
					ImGui::Separator();
					
					// Component header
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
					bool box_collider_open = ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleColor();
					
					ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
					if (ImGui::SmallButton(" - ##box_collider_component")) {
						m_Entity.RemoveComponent<BoxColliderComponent>();
					}
					
					if (box_collider_open) {
						BoxColliderComponent& box_collider_component = m_Entity.GetComponent<BoxColliderComponent>();

						// Remove window padding for component table
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
						
						if(ImGui::BeginTable("##box_collider_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
						{
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

							// Size
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Size");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							if (ImGui::DragFloat3("##box_size", (float32*)&box_collider_component.size, 0.01f, 0.01f, FLT_MAX, "%.2f")) {
								if (box_collider_component.size.x < 0.01f) box_collider_component.size.x = 0.01f;
								if (box_collider_component.size.y < 0.01f) box_collider_component.size.y = 0.01f;
								if (box_collider_component.size.z < 0.01f) box_collider_component.size.z = 0.01f;
							}
							
							if (ImGui::IsItemActive()) {
								TRSComponent trs_component = m_Entity.GetWorldTransform();
								// Multiply box collider size by 2 for rendering because Jolt takes half-size
								DebugRenderer::RenderWireframeBox(trs_component.translation, trs_component.rotation, box_collider_component.size * 2.0f, { 0.28f, 0.27f, 1.0f });
							}

							// convex radius
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Radius");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::SliderFloat("##box_radius", &box_collider_component.convex_radius, 0.0f, 25.0f, "%.2f");

							// Friction
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Friction");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::SliderFloat("##box_friction", &box_collider_component.friction, 0.0f, 1.0f, "%.2f");

							// Restitution
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Bounce");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::SliderFloat("##box_restitution", &box_collider_component.restitution, 0.0f, 1.0f, "%.2f");

							ImGui::EndTable();
						}
						ImGui::PopStyleVar(2);
					}
				}
				if (m_Entity.HasComponent<SphereColliderComponent>()) {
					ImGui::Spacing();
					ImGui::Separator();
					
					// Component header
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
					bool sphere_collider_open = ImGui::CollapsingHeader("Sphere Collider", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleColor();
					
					ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
					if (ImGui::SmallButton(" - ##sphere_collider_component")) {
						m_Entity.RemoveComponent<SphereColliderComponent>();
					}
					
					if (sphere_collider_open) {
						SphereColliderComponent& sphere_collider_component = m_Entity.GetComponent<SphereColliderComponent>();

						// Remove window padding for component table
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
						
						if(ImGui::BeginTable("##sphere_collider_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
						{
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

							// Radius
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Radius");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							if (ImGui::DragFloat("##sphere_radius", (float32*)&sphere_collider_component.radius, 0.01f, 0.01f, FLT_MAX, "%.2f"))
								if (sphere_collider_component.radius < 0.01f) sphere_collider_component.radius = 0.01f;

							if (ImGui::IsItemActive()) {
								TRSComponent trs_component = m_Entity.GetWorldTransform();
								DebugRenderer::RenderWireframeSphere(trs_component.translation, sphere_collider_component.radius, { 0.28f, 0.27f, 1.0f });
							}

							// Friction
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Friction");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::SliderFloat("##sphere_friction", &sphere_collider_component.friction, 0.0f, 1.0f, "%.2f");

							// Restitution
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Bounce");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::SliderFloat("##sphere_restitution", &sphere_collider_component.restitution, 0.0f, 1.0f, "%.2f");

							ImGui::EndTable();
						}
						ImGui::PopStyleVar(2);
					}
				}
				if (m_Entity.HasComponent<ScriptComponent>()) {
					ImGui::Spacing();
					ImGui::Separator();
					
					// Component header
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
					bool script_open = ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleColor();
					
					ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
					if (ImGui::SmallButton(" - ##script_component")) {
						m_Entity.RemoveComponent<ScriptComponent>();
					}
					
					if (script_open) {
						ScriptComponent& script_component = m_Entity.GetComponent<ScriptComponent>();

						// Remove window padding for component table
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
						
						if (ImGui::BeginTable("##script_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV)) {
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Class Name");

							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::InputText("##script_class", script_component.class_name.data(), script_component.class_name.capacity());
							script_component.class_name.resize(strlen(script_component.class_name.c_str()));
							ImGui::EndTable();
						}
						ImGui::PopStyleVar(2);
					}
				}
				if (m_Entity.HasComponent<PointLightComponent>()) {
					ImGui::Spacing();
					ImGui::Separator();
					
					// Component header
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
					bool light_open = ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen);
					ImGui::PopStyleColor();
					
					ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
					if (ImGui::SmallButton(" - ##point_light_component")) {
						m_Entity.RemoveComponent<PointLightComponent>();
					}
					
					if (light_open) {
						PointLightComponent& point_light_component = m_Entity.GetComponent<PointLightComponent>();

						// Remove window padding for component table
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 4.0f));
						
						if (ImGui::BeginTable("##point_light_properties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
						{
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Intensity");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::DragFloat("##light_intensity", &point_light_component.intensity, 0.01f, 0.0f, FLT_MAX, "%.2f");

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Radius");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							if (ImGui::DragFloat("##light_radius", &point_light_component.radius, 0.05f, 0.1f, FLT_MAX, "%.2f"))
								if (point_light_component.radius < 0.1f) point_light_component.radius = 0.1f;
							
							if (ImGui::IsItemActive()) {
								DebugRenderer::RenderWireframeSphere(m_Entity.GetWorldTransform().translation, point_light_component.radius, { 0.28f, 0.27f, 1.0f });
							}

							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Color");
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1.0f);
							ImGui::ColorEdit3("##light_color", (float*)&point_light_component.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

							ImGui::EndTable();
						}
						ImGui::PopStyleVar(2);
					}
				}
			}
			else {
				// No entity selected - show helpful message
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
				
				ImVec2 window_size = ImGui::GetWindowSize();
				ImVec2 text_size = ImGui::CalcTextSize("No entity selected");
				ImGui::SetCursorPos(ImVec2((window_size.x - text_size.x) * 0.5f, window_size.y * 0.4f));
				ImGui::Text("No entity selected");
				
				text_size = ImGui::CalcTextSize("Select an entity in the hierarchy to view its properties");
				ImGui::SetCursorPos(ImVec2((window_size.x - text_size.x) * 0.5f, window_size.y * 0.4f + 25.0f));
				ImGui::Text("Select an entity in the hierarchy to view its properties");
				
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			}
            ImGui::End();
		}
	}

	void PropertiesPanel::SetEntity(Entity entity, bool selected)
	{
		m_Entity = entity; m_Selected = selected;
	}

}