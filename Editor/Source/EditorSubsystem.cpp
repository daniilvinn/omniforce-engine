#include <Omniforce.h>

#include "EditorPanels/SceneHierarchy.h"
#include "EditorPanels/Properties.h"
#include "EditorPanels/ContentBrowser.h"
#include "EditorPanels/Logs.h"
#include "EditorPanels/PathTracingSettings.h"

#include "EditorCamera.h"

#include <filesystem>
#include <fstream>

#include <tinyfiledialogs.h>
#include <ImGuizmo.h>

using namespace Omni;

class EditorSubsystem : public Subsystem
{
public:
	~EditorSubsystem() override
	{
		Destroy();
	}

	void OnUpdate(float32 step) override
	{
		// Main menu bar
		ImGui::BeginMainMenuBar();
		if (ImGui::MenuItem("File")) {
			ImGui::OpenPopup("menu_bar_file");
		};
		if (ImGui::MenuItem("View")) {
			ImGui::OpenPopup("menu_bar_view");
		};
		if (ImGui::BeginPopup("menu_bar_file")) {
			if (ImGui::MenuItem("Open project", "Ctrl + O")) {
				LoadProject();
			};
			if (ImGui::MenuItem("Save project", "Ctrl + S")) {
				SaveProject();
			};
			if (ImGui::MenuItem("New project", "Ctrl + N")) {
				NewProject();
			};
			ImGui::EndPopup();
		};
		if (ImGui::BeginPopup("menu_bar_view")) {
			if (ImGui::MenuItem("Scene hierarchy")) {
				m_HierarchyPanel->Open(true);
			}
			if (ImGui::MenuItem("Properties")) {
				m_PropertiesPanel->Open(true);
			}
			if (ImGui::MenuItem("Content browser")) {
				m_AssetsPanel->Open(true);
			}
			if (ImGui::MenuItem("Path Tracing Settings")) {
				m_PathTracingPanel->Open(true);
			}
			ImGui::EndPopup();
		}
		ImGui::EndMainMenuBar();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		//Render and process scene hierarchy panel
		ImGui::BeginDisabled(m_InRuntime);
		m_HierarchyPanel->Update();
		if (m_EntitySelected = m_HierarchyPanel->IsNodeSelected()) 
		{
			m_SelectedEntity = m_HierarchyPanel->GetSelectedNode();
		}

		// Properties panel
		m_PropertiesPanel->SetEntity(m_SelectedEntity, m_HierarchyPanel->IsNodeSelected());
		m_PropertiesPanel->Update();

		// Asset panel
		m_AssetsPanel->Update();

		// Logs panel
		m_LogsPanel->Update();

		// Path Tracing Settings panel
		m_PathTracingPanel->Update();

		// Debug
		ImGui::Begin("Debug");
		ImGui::Text(fmt::format("Delta time: {}", step * 1000.0f).c_str());
		ImGui::Text(fmt::format("FPS: {}", (uint32)(1000.0f / (step * 1000.0f))).c_str());
		ImGui::End();

		// Utils
		ImGui::Begin("Utils");
		{
			// TODO: fix bug here and in properties panel
			// when engine crashes after trying to close / hide imgui window which contains tables.
			PhysicsSettings physics_settings = m_CurrentScene->GetPhysicsSettings();
			ImGui::Text("Gravity");
			ImGui::SameLine();

			if(ImGui::DragFloat3("##physics_settings_gravity_drag_float", (float32*)&physics_settings.gravity, 0.01f, -99.0f, 99.0f))
				m_CurrentScene->SetPhysicsSettings(physics_settings);

			if (ImGui::Button("Reload script assemblies"))
				ScriptEngine::Get()->ReloadAssemblies();

			ImGui::Checkbox("Visualize physics colliders", &m_VisualizeColliders);
			ImGui::Checkbox("Visualize mesh cull bounds", &m_VisualizeCullBounds);
			ImGui::Checkbox("Scene cluster debug view", &m_SceneDebugViewEnabled);

			if (m_VisualizeColliders) {
				auto box_colliders_view = m_EditorScene->GetRegistry()->view<BoxColliderComponent>();
				for (auto& e : box_colliders_view) {
					Entity entity(e, m_CurrentScene);
					// Not world transform, since entities with rigid body must be not a child of another entity
					const TRSComponent& trs = entity.GetComponent<TRSComponent>();
					const BoxColliderComponent& bc_component = entity.GetComponent<BoxColliderComponent>();

					DebugRenderer::RenderWireframeBox(trs.translation, trs.rotation, bc_component.size * 2.0f, { 0.28f, 0.27f, 1.0f });
				}

				auto sphere_colliders_view = m_EditorScene->GetRegistry()->view<SphereColliderComponent>();
				for (auto& e : sphere_colliders_view) {
					Entity entity(e, m_CurrentScene);
					// Not world transform, since entities with rigid body must be not a child of another entity
					const TRSComponent& trs = entity.GetComponent<TRSComponent>();
					const SphereColliderComponent& sc_component = entity.GetComponent<SphereColliderComponent>();

					DebugRenderer::RenderWireframeSphere(trs.translation, sc_component.radius, { 0.28f, 0.27f, 1.0f });
				}
			}
			if (m_VisualizeCullBounds) {
				auto mesh_view = m_CurrentScene->GetRegistry()->view<MeshComponent>();
				for (auto& e : mesh_view) {
					Entity entity(e, m_CurrentScene);

					const TRSComponent trs = entity.GetWorldTransform();
					const MeshComponent& mesh_component = entity.GetComponent<MeshComponent>();

					Ref<Mesh> mesh = AssetManager::Get()->GetAsset<Mesh>(mesh_component.mesh_handle);
					Sphere bounding_sphere = mesh->GetBoundingSphere();
					AABB aabb = mesh->GetAABB();

					float32 max_scale = glm::max(glm::max(trs.scale.x, trs.scale.y), trs.scale.z);

					DebugRenderer::RenderWireframeSphere(
						trs.translation + bounding_sphere.center * trs.scale,
						bounding_sphere.radius * max_scale,
						{ 0.28f, 0.27f, 1.0f }
					);

					glm::vec3 aabb_scale = glm::vec3{
						(aabb.max.x - aabb.min.x),
						(aabb.max.y - aabb.min.y),
						(aabb.max.z - aabb.min.z)
					};

					glm::vec3 aabb_translation = glm::vec3{
						(aabb.min.x + aabb.max.x) * 0.5f + trs.translation.x,
						(aabb.min.y + aabb.max.y) * 0.5f + trs.translation.y,
						(aabb.min.z + aabb.max.z) * 0.5f + trs.translation.z
					};

					DebugRenderer::RenderWireframeBox(aabb_translation, trs.rotation, aabb_scale, { 0.28f, 0.27f, 1.0f });
				}
			}
			if (m_SceneDebugViewEnabled) {
				if (!m_CurrentScene->GetRenderer()->IsInDebugMode()) {
					m_CurrentScene->GetRenderer()->EnterDebugMode(DebugSceneView::CLUSTER);
				}
				
				const char* items[] = { "Cluster view", "Triangle view", "Cluster group"};
				uint32 current_item = uint32(m_CurrentScene->GetRenderer()->GetCurrentDebugMode());
				if (ImGui::BeginCombo("View", items[current_item])) {

					for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
						bool is_selected = current_item == i;

						switch (i)
						{
						case 0:  ImGui::Selectable("Cluster view", &is_selected);		break;
						case 1:  ImGui::Selectable("Triangle view", &is_selected);		break;
						default: break;
						}

						if (is_selected) {
							m_CurrentScene->GetRenderer()->EnterDebugMode(DebugSceneView(i));
							current_item = i;
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}
			}
			else {
				m_CurrentScene->GetRenderer()->ExitDebugMode();
			}

		}
		ImGui::End();
		ImGui::EndDisabled();

		// Render and process play/stop button
		ImGui::Begin("##Toolbar", nullptr, 
			ImGuiWindowFlags_NoDecoration | 
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | 
			ImGuiWindowFlags_NoTitleBar
		);
		if (ImGui::Button(m_InRuntime ? "Stop" : "Play")) {
			m_InRuntime = !m_InRuntime;

			Omni::UUID selected_node;
			if(m_EntitySelected)
				selected_node = m_SelectedEntity.GetComponent<UUIDComponent>();

			if (m_InRuntime) {
				m_RuntimeScene = new Scene(m_EditorScene); 
				m_RuntimeScene->LaunchRuntime();
				m_CurrentScene = m_RuntimeScene;
			}
			else {
				m_RuntimeScene->ShutdownRuntime();
				if (m_RuntimeScene)
					delete m_RuntimeScene;
				m_CurrentScene = m_EditorScene;
				m_CurrentScene->EditorSetCamera(m_EditorCamera);
			};

			if (m_EntitySelected) {
				entt::entity entity_id = m_CurrentScene->GetEntities().at(selected_node);
				m_SelectedEntity = Entity(m_SelectedEntity, m_CurrentScene);
			}
			m_HierarchyPanel->SetContext(m_CurrentScene);
			m_HierarchyPanel->SetSelectedNode(m_SelectedEntity, m_EntitySelected);
			m_PropertiesPanel->SetContext(m_CurrentScene);
		};
		ImGui::End();

		// Viewport panel
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		ImGui::Begin("Viewport");
		{
			ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
			ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			ImVec2 viewportOffset = ImGui::GetWindowPos();
			m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			m_ViewportFocused = ImGui::IsWindowFocused();
			ImVec2 viewport_frame_size = ImGui::GetContentRegionAvail();
			UI::RenderImage(m_EditorScene->GetFinalImage(), m_EditorScene->GetRenderer()->GetSamplerLinear(), viewport_frame_size, 0, true);

			if(auto camera = m_CurrentScene->GetCamera(); camera)
				camera->SetAspectRatio(viewport_frame_size.x / viewport_frame_size.y);
			
			if (ImGui::BeginDragDropTarget()) {
				ImGuiDragDropFlags target_flags = 0;
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("content_browser_item", target_flags);

				if (payload) {
					std::filesystem::path filename(std::string((char*)payload->Data, payload->DataSize));
					if (filename.extension() == ".gltf" || filename.extension() == ".glb") {
						AssetManager* asset_manager = AssetManager::Get();
						ModelImporter importer;
						Ref<Model> model = AssetManager::Get()->GetAsset<Model>(importer.Import(filename));

						Entity root_entity = m_CurrentScene->CreateEntity();

						auto& children_map = model->GetMap();
						for (auto& entry : children_map) {
							Entity child = m_CurrentScene->CreateChildEntity(root_entity);
							child.GetComponent<TagComponent>().tag = asset_manager->GetAsset<Material>(entry.second)->GetName();
							MeshComponent& mesh_component = child.AddComponent<MeshComponent>();
							mesh_component.mesh_handle = entry.first;
							mesh_component.material_handle = entry.second;

							// TODO: definitely need to move it from here to somewhere else into engine core
							m_EditorScene->GetRenderer()->AcquireResourceIndex(asset_manager->GetAsset<Mesh>(mesh_component.mesh_handle));
							m_EditorScene->GetRenderer()->AcquireResourceIndex(asset_manager->GetAsset<Material>(mesh_component.material_handle));
						}
					}
				}
				
				ImGui::EndDragDropTarget();
			}

			if (m_InRuntime) {
				m_CurrentOperation = (ImGuizmo::OPERATION)0;
			}
			else {
				RenderGizmos();
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();

		// Update editor camera and scene
		m_CurrentScene->OnUpdate(step);
		if(m_ViewportFocused && !m_InRuntime)
			m_EditorCamera->OnUpdate(step);
	}

	void Launch() override
	{
		m_EditorScene = new Scene(SceneType::SCENE_TYPE_3D);

		m_HierarchyPanel = CreatePtr<SceneHierarchyPanel>(&g_PersistentAllocator, m_EditorScene);
		m_PropertiesPanel = CreatePtr<PropertiesPanel>(&g_PersistentAllocator, m_EditorScene);
		m_AssetsPanel = CreatePtr<ContentBrowser>(&g_PersistentAllocator, m_EditorScene);
		m_LogsPanel = CreatePtr<LogsPanel>(&g_PersistentAllocator, m_EditorScene);
		m_PathTracingPanel = CreatePtr<PathTracingSettingsPanel>(&g_PersistentAllocator, m_EditorScene);

		m_ProjectPath = "";

		m_EditorCamera = CreateRef<EditorCamera>(&g_PersistentAllocator, 16.0 / 9.0);
		m_EditorScene->EditorSetCamera(m_EditorCamera);
		m_CurrentScene = m_EditorScene;

		ImGuizmo::SetOrthographic(false);
		
		m_ProjectPath = "resources/SandboxProject";
		m_ProjectFilename = "Sandbox.omni";
		FileSystem::SetWorkingDirectory(m_ProjectPath);
	}

	void Destroy() override
	{
		delete m_EditorScene;
	}

	void OnEvent(Event* e) override
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnWindowResize));
		dispatcher.Dispatch<KeyPressedEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnKeyPressed));

		if (m_ViewportFocused)
			m_EditorCamera->OnEvent(e);
	}

	bool OnWindowResize(WindowResizeEvent* e) {
		return false;
	}

	bool OnKeyPressed(KeyPressedEvent* e) {
		if (!e->GetRepeatCount()) {
			if (Input::KeyPressed(KeyCode::KEY_LEFT_CONTROL) || Input::KeyPressed(KeyCode::KEY_RIGHT_CONTROL)) {
				if (Input::KeyPressed(KeyCode::KEY_S))
					SaveProject();
				if (Input::KeyPressed(KeyCode::KEY_O))
					LoadProject();
				if (Input::KeyPressed(KeyCode::KEY_N))
					NewProject();
				if (Input::KeyPressed(KeyCode::KEY_Q))
					m_CurrentOperation = (ImGuizmo::OPERATION)0;
				if (Input::KeyPressed(KeyCode::KEY_W))
					m_CurrentOperation = ImGuizmo::OPERATION::TRANSLATE;
				if (Input::KeyPressed(KeyCode::KEY_E))
					m_CurrentOperation = ImGuizmo::OPERATION(ImGuizmo::OPERATION::ROTATE & ~(ImGuizmo::OPERATION::ROTATE_SCREEN));
				if (Input::KeyPressed(KeyCode::KEY_R))
					m_CurrentOperation = ImGuizmo::OPERATION::SCALE;
			}
		}
		return false;
	}

	void SaveProject() {
		if (m_ProjectPath.empty()) {
			NewProject(); // it will still lead to calling SaveProject() second time, so we can return
			return;
		}

		nlohmann::json root_node = {};
		m_EditorScene->Serialize(root_node);

		std::ofstream out_stream(m_ProjectPath.string() + "/" + m_ProjectFilename);
		out_stream << root_node.dump(4);
		out_stream.close();
		
	};

	void LoadProject() 
	{
		const char* filters[] = { "*.omni" };

		const char* filepath = tinyfd_openFileDialog(
			"Open project",
			std::filesystem::current_path().string().c_str(),
			1,
			filters,
			"Omniforce project files (*.omni)",
			false
		);

		if (filepath == NULL)
			return;

		std::ifstream input(filepath);
		nlohmann::json root_node;
		try
		{
			root_node = nlohmann::json::parse(input);
		}
		catch (const std::exception&)
		{
			OMNIFORCE_CLIENT_ERROR("Failed to load project at location: {}", filepath);
			return;
		}
		input.close();

		m_ProjectPath = filepath;
		m_ProjectFilename = m_ProjectPath.filename().string();
		m_ProjectPath.remove_filename();

		FileSystem::SetWorkingDirectory(m_ProjectPath);
			
		Renderer::WaitDevice();

		// unloading textures from memory and releasing their indices
		AssetManager* asset_manager = AssetManager::Get();
		auto renderer = m_EditorScene->GetRenderer();
		auto& texture_registry = *asset_manager->GetAssetRegistry();
		for (auto [id, asset] : texture_registry) {
			if(asset->Type != AssetType::OMNI_IMAGE)
				continue;
			renderer->ReleaseResourceIndex(asset);
		}
		asset_manager->FullUnload();

		m_EditorScene->Deserialize(root_node);
		m_EditorScene->EditorSetCamera(m_EditorCamera);
		m_HierarchyPanel->SetContext(m_EditorScene);
		m_HierarchyPanel->SetSelectedNode({ (entt::entity)0, m_CurrentScene }, false);
		m_AssetsPanel->SetContext(m_EditorScene);

		ScriptEngine* script_engine = ScriptEngine::Get();
		if (script_engine->HasAssemblies())
			script_engine->UnloadAssemblies();
		script_engine->LoadAssemblies();
	};

	void NewProject() {
		const char* filters[] = { "*.omni" };

		const char* filepath = tinyfd_saveFileDialog(
			"New project",
			std::filesystem::current_path().string().c_str(),
			1,
			filters,
			nullptr
		);

		if (filepath == NULL)
			return;

		m_ProjectPath = filepath;
		m_ProjectFilename = m_ProjectPath.filename().string();
		m_ProjectPath.remove_filename();

		FileSystem::SetWorkingDirectory(m_ProjectPath);

		auto textures_dir = m_ProjectPath.string() + "Assets/Textures";
		auto scripts_dir = m_ProjectPath.string() +  "Assets/Scripts/Assemblies";
		auto audio_dir = m_ProjectPath.string() + "Assets/Audio";
		auto mesh_dir = m_ProjectPath.string() + "Assets/Meshes";

		std::filesystem::create_directories(textures_dir);
		std::filesystem::create_directories(scripts_dir);
		std::filesystem::create_directories(audio_dir);
		std::filesystem::create_directories(mesh_dir);

		std::filesystem::copy("Resources/Scripts/ScriptsProject", m_ProjectPath.string() + "/Assets/Scripts", std::filesystem::copy_options::recursive);
		std::filesystem::copy("Resources/Scripting/Build/ScriptEngine.dll", m_ProjectPath / "Assets/Scripts/Assemblies/ScriptEngine.dll");

		if(m_ProjectPath.string().length())
			SaveProject();
	}

	void RenderGizmos() {
		if (m_EntitySelected && m_CurrentOperation) {
			TRSComponent trs = m_SelectedEntity.GetWorldTransform();

			glm::mat4 model = Utils::ComposeMatrix(trs.translation, trs.rotation, trs.scale);
			glm::mat4 view = m_EditorCamera->GetViewMatrix();
			glm::mat4 proj = m_EditorCamera->BuildNonReversedProjection();

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(
				m_ViewportBounds[0].x,
				m_ViewportBounds[0].y,
				m_ViewportBounds[1].x - m_ViewportBounds[0].x,
				m_ViewportBounds[1].y - m_ViewportBounds[0].y
			);

			bool needs_snapping = Input::KeyPressed(KeyCode::KEY_LEFT_ALT);
			float snap_value[2] = { 0.0f, 0.0f };
			if (needs_snapping) {
				if (m_CurrentOperation & ImGuizmo::OPERATION::ROTATE_Z) {
					snap_value[0] = 45.0f;
				}

				else {
					snap_value[0] = 0.5f;
					snap_value[1] = 0.5f;
				}
			}

			ImGuizmo::Manipulate(
				(float*)&view,
				(float*)&proj,
				m_CurrentOperation,
				ImGuizmo::MODE::WORLD,
				(float*)&model,
				nullptr,
				snap_value
			);

			if (ImGuizmo::IsUsing()) {
				TRSComponent& trs_component = m_SelectedEntity.GetComponent<TRSComponent>();

				if (m_SelectedEntity.GetComponent<HierarchyNodeComponent>().parent.Valid()) {
					Entity parent_entity = m_SelectedEntity.GetParent();

					TRSComponent parent_trs = parent_entity.GetWorldTransform();

					glm::mat4 parent_transform = Utils::ComposeMatrix(parent_trs.translation, parent_trs.rotation, glm::vec3(1.0f));

					model = glm::inverse(parent_transform) * model;
				}

				Utils::DecomposeMatrix(model, &trs_component.translation, &trs_component.rotation, &trs_component.scale);

			}

		}
	}



	/*
	*	DATA
	*/
	Scene* m_EditorScene;
	Scene* m_RuntimeScene = nullptr;
	Scene* m_CurrentScene;
	Ref<EditorCamera> m_EditorCamera;
	Entity m_SelectedEntity;
	bool m_EntitySelected = false;
	bool m_ViewportFocused = false;
	bool m_InRuntime = false;
	bool m_VisualizeColliders = false;
	bool m_VisualizeCullBounds = false;
	bool m_SceneDebugViewEnabled = false;

	Ptr<SceneHierarchyPanel> m_HierarchyPanel;
	Ptr<PropertiesPanel> m_PropertiesPanel;
	Ptr<ContentBrowser> m_AssetsPanel;
	Ptr<LogsPanel> m_LogsPanel;
	Ptr<PathTracingSettingsPanel> m_PathTracingPanel;

	std::filesystem::path m_ProjectPath;
	std::string m_ProjectFilename;
	ImGuizmo::OPERATION m_CurrentOperation = (ImGuizmo::OPERATION)0;

	fvec2 m_ViewportBounds[2];

};

Ptr<Subsystem> ConstructRootSystem()
{
	return CreatePtr<EditorSubsystem>(&g_PersistentAllocator);
}