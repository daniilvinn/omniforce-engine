#include <Omniforce.h>

#include "EditorPanels/SceneHierarchy.h"
#include "EditorPanels/Properties.h"
#include "EditorPanels/ContentBrowser.h"
#include "EditorPanels/Logs.h"
#include "EditorPanels/PathTracingSettings.h"

#include "EditorCamera.h"

#include <filesystem>
// #include <fstream>

#include <tinyfiledialogs.h>
#include <ImGuizmo.h>

// Editor refactor services and manager
#include "PanelManager.h"
#include "Services/EditorContext.h"
#include "Services/SelectionService.h"
#include "Services/GizmoController.h"
#include <memory>
#include "Services/ProjectService.h"
#include "EditorPanels/ViewportPanel.h"

using namespace Omni;
using namespace Omni::EditorServices;

class EditorSubsystem : public Subsystem {
public:
    ~EditorSubsystem() override { Destroy(); }

    void OnUpdate(float32 step) override {
        DrawMainMenuBar();
        DrawDockspace();
        DrawToolbar();
        // Update viewport panel explicitly so it renders the scene and handles DnD/gizmos
        if (auto* vp = m_PanelManager->GetPanelAs<ViewportPanel>("viewport")) {
            vp->Update();
        }
        DrawViewport();
        DrawDebugWindow(step);
        DrawUtilsWindow();

        // Panels in a defined order to preserve prior behavior
        ImGui::BeginDisabled(m_Context.IsInRuntime());
        auto* hierarchy = static_cast<SceneHierarchyPanel*>(m_PanelManager->GetPanel("scene_hierarchy"));
        auto* properties = static_cast<PropertiesPanel*>(m_PanelManager->GetPanel("properties"));
        auto* content = static_cast<ContentBrowser*>(m_PanelManager->GetPanel("content_browser"));
        auto* logs = static_cast<LogsPanel*>(m_PanelManager->GetPanel("logs"));
        auto* pt = static_cast<PathTracingSettingsPanel*>(m_PanelManager->GetPanel("path_tracing_settings"));

        if (hierarchy) {
            hierarchy->Update();
        }
        if (hierarchy && hierarchy->IsNodeSelected()) {
            m_SelectionService->SetSelected(hierarchy->GetSelectedNode(), true);
        }
        else {
            m_SelectionService->Clear();
        }

        if (properties) {
            properties->SetEntity(m_SelectionService->GetSelected(), m_SelectionService->HasSelection());
            properties->Update();
        }
        if (content) {
            content->Update();
        }
#if 1
        if (logs) {
            logs->Update();
        }
#endif
        if (pt) {
            pt->Update();
        }
        ImGui::EndDisabled();

        // Update scene and camera
        m_Context.GetCurrentScene()->OnUpdate(step);
        if (m_Context.IsViewportFocused() && !m_Context.IsInRuntime()) {
            m_Context.GetEditorCamera()->OnUpdate(step);
        }
    }

    void Launch() override {
        Scene* editor_scene = new Scene(SceneType::SCENE_TYPE_3D);
        m_Context.SetEditorScene(editor_scene);
        m_Context.SetCurrentScene(editor_scene);

        Ref<EditorCamera> camera = CreateRef<EditorCamera>(&g_PersistentAllocator, 16.0 / 9.0);
        editor_scene->EditorSetCamera(camera);
        m_Context.SetEditorCamera(camera);

        ImGuizmo::SetOrthographic(false);

        // Services
        m_SelectionService = std::make_unique<SelectionService>();
        m_GizmoController = std::make_unique<GizmoController>();
        m_GizmoController->SetContext(&m_Context);
        m_Context.SetSelectionService(m_SelectionService.get());

        // Panel manager
        if (!::Omni::PanelManager::Get()) ::Omni::PanelManager::Init();
        m_PanelManager = ::Omni::PanelManager::Get();
        m_PanelManager->SetContext(editor_scene);
        m_PanelManager->SetEditorContext(&m_Context);
        m_PanelManager->AddPanel("logs", new LogsPanel(editor_scene));
        m_PanelManager->AddPanel("path_tracing_settings", new PathTracingSettingsPanel(editor_scene));
        m_PanelManager->AddPanel("viewport", new ViewportPanel(editor_scene, m_GizmoController.get()));

        // Project defaults and project service
        m_ProjectService = std::make_unique<ProjectService>();
        m_ProjectService->Initialize(&m_Context, m_PanelManager);
        m_Context.SetProjectPath("resources/SandboxProject");
        m_Context.SetProjectFilename("Sandbox.omni");
        FileSystem::SetWorkingDirectory(m_Context.GetProjectPath());
    }

    void Destroy() override {
        delete m_Context.GetEditorScene();
    }

    void OnEvent(Event* e) override {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnWindowResize));
        dispatcher.Dispatch<KeyPressedEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnKeyPressed));

        if (m_Context.IsViewportFocused())
            m_Context.GetEditorCamera()->OnEvent(e);
    }

    bool OnWindowResize(WindowResizeEvent* /*e*/) { return false; }

    bool OnKeyPressed(KeyPressedEvent* e) {
        if (!e->GetRepeatCount()) {
            if (Input::KeyPressed(KeyCode::KEY_LEFT_CONTROL) || Input::KeyPressed(KeyCode::KEY_RIGHT_CONTROL)) {
                if (Input::KeyPressed(KeyCode::KEY_S)) m_ProjectService->SaveProject();
                if (Input::KeyPressed(KeyCode::KEY_O)) m_ProjectService->LoadProject();
                if (Input::KeyPressed(KeyCode::KEY_N)) m_ProjectService->NewProject();
                if (Input::KeyPressed(KeyCode::KEY_Q)) m_GizmoController->SetOperation((ImGuizmo::OPERATION)0);
                if (Input::KeyPressed(KeyCode::KEY_W)) m_GizmoController->SetOperation(ImGuizmo::OPERATION::TRANSLATE);
                if (Input::KeyPressed(KeyCode::KEY_E)) m_GizmoController->SetOperation(ImGuizmo::OPERATION(ImGuizmo::OPERATION::ROTATE & ~(ImGuizmo::OPERATION::ROTATE_SCREEN)));
                if (Input::KeyPressed(KeyCode::KEY_R)) m_GizmoController->SetOperation(ImGuizmo::OPERATION::SCALE);
            }
        }
        return false;
    }

private:
    void DrawMainMenuBar() {
        ImGui::BeginMainMenuBar();
        if (ImGui::MenuItem("File")) ImGui::OpenPopup("menu_bar_file");
        if (ImGui::MenuItem("View")) ImGui::OpenPopup("menu_bar_view");
        if (ImGui::BeginPopup("menu_bar_file")) {
            if (ImGui::MenuItem("Open project", "Ctrl + O")) m_ProjectService->LoadProject();
            if (ImGui::MenuItem("Save project", "Ctrl + S")) m_ProjectService->SaveProject();
            if (ImGui::MenuItem("New project", "Ctrl + N")) m_ProjectService->NewProject();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopup("menu_bar_view")) {
            if (auto* p = m_PanelManager->GetPanel("scene_hierarchy")) { if (ImGui::MenuItem("Scene hierarchy")) p->Open(true); }
            if (auto* p = m_PanelManager->GetPanel("properties")) { if (ImGui::MenuItem("Properties")) p->Open(true); }
            if (auto* p = m_PanelManager->GetPanel("content_browser")) { if (ImGui::MenuItem("Content browser")) p->Open(true); }
            if (auto* p = m_PanelManager->GetPanel("viewport")) { if (ImGui::MenuItem("Viewport")) p->Open(true); }
            if (auto* p = m_PanelManager->GetPanel("logs")) { if (ImGui::MenuItem("Logs")) p->Open(true); }
            if (auto* p = m_PanelManager->GetPanel("path_tracing_settings")) { if (ImGui::MenuItem("Path Tracing Settings")) p->Open(true); }
            ImGui::EndPopup();
        }
        ImGui::EndMainMenuBar();
    }

    void DrawDockspace() { ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()); }

    void DrawToolbar() {
        ImGui::Begin("##Toolbar", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoTitleBar);
        bool in_runtime = m_Context.IsInRuntime();
        if (ImGui::Button(in_runtime ? "Stop" : "Play")) {
            ToggleRuntime();
        }
        ImGui::End();
    }

    void DrawViewport() { /* handled by ViewportPanel */ }

    void DrawDebugWindow(float32 step) {
        ImGui::Begin("Debug");
        ImGui::Text("%s", fmt::format("Delta time: {}", step * 1000.0f).c_str());
        ImGui::Text("%s", fmt::format("FPS: {}", (uint32)(1000.0f / (step * 1000.0f))).c_str());
        ImGui::End();
    }

    void DrawUtilsWindow() {
        ImGui::Begin("Utils");
        {
            PhysicsSettings physics_settings = m_Context.GetCurrentScene()->GetPhysicsSettings();
            ImGui::Text("Gravity");
            ImGui::SameLine();
            if (ImGui::DragFloat3("##physics_settings_gravity_drag_float", (float32*)&physics_settings.gravity, 0.01f, -99.0f, 99.0f))
                m_Context.GetCurrentScene()->SetPhysicsSettings(physics_settings);

            if (ImGui::Button("Reload script assemblies"))
                ScriptEngine::Get()->ReloadAssemblies();

            ImGui::Checkbox("Visualize physics colliders", &m_Context.VisualizeColliders());
            ImGui::Checkbox("Visualize mesh cull bounds", &m_Context.VisualizeCullBounds());
            ImGui::Checkbox("Scene cluster debug view", &m_Context.SceneDebugViewEnabled());

            if (m_Context.VisualizeColliders()) {
                auto box_colliders_view = m_Context.GetEditorScene()->GetRegistry()->view<BoxColliderComponent>();
                for (auto& e : box_colliders_view) {
                    Entity entity(e, m_Context.GetCurrentScene());
                    const TRSComponent& trs = entity.GetComponent<TRSComponent>();
                    const BoxColliderComponent& bc_component = entity.GetComponent<BoxColliderComponent>();
                    DebugRenderer::RenderWireframeBox(trs.translation, trs.rotation, bc_component.size * 2.0f, { 0.28f, 0.27f, 1.0f });
                }

                auto sphere_colliders_view = m_Context.GetEditorScene()->GetRegistry()->view<SphereColliderComponent>();
                for (auto& e : sphere_colliders_view) {
                    Entity entity(e, m_Context.GetCurrentScene());
                    const TRSComponent& trs = entity.GetComponent<TRSComponent>();
                    const SphereColliderComponent& sc_component = entity.GetComponent<SphereColliderComponent>();
                    DebugRenderer::RenderWireframeSphere(trs.translation, sc_component.radius, { 0.28f, 0.27f, 1.0f });
                }
            }
            if (m_Context.VisualizeCullBounds()) {
                auto mesh_view = m_Context.GetCurrentScene()->GetRegistry()->view<MeshComponent>();
                for (auto& e : mesh_view) {
                    Entity entity(e, m_Context.GetCurrentScene());
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
            if (m_Context.SceneDebugViewEnabled()) {
                if (!m_Context.GetCurrentScene()->GetRenderer()->IsInDebugMode()) {
                    m_Context.GetCurrentScene()->GetRenderer()->EnterDebugMode(DebugSceneView::CLUSTER);
                }
                const char* items[] = { "Cluster view", "Triangle view", "Cluster group"};
                uint32 current_item = uint32(m_Context.GetCurrentScene()->GetRenderer()->GetCurrentDebugMode());
                if (ImGui::BeginCombo("View", items[current_item])) {
                    for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
                        bool is_selected = current_item == i;
                        switch (i) {
                        case 0:  ImGui::Selectable("Cluster view", &is_selected); break;
                        case 1:  ImGui::Selectable("Triangle view", &is_selected); break;
                        default: break;
                        }
                        if (is_selected) {
                            m_Context.GetCurrentScene()->GetRenderer()->EnterDebugMode(DebugSceneView(i));
                            current_item = i;
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            } else {
                m_Context.GetCurrentScene()->GetRenderer()->ExitDebugMode();
            }
        }
        ImGui::End();
    }

    void ToggleRuntime() {
        bool enter_runtime = !m_Context.IsInRuntime();
        m_Context.SetInRuntime(enter_runtime);

        Omni::UUID selected_node;
        if (m_SelectionService->HasSelection())
            selected_node = m_SelectionService->GetSelected().GetComponent<UUIDComponent>();

        if (enter_runtime) {
            Scene* runtime = new Scene(m_Context.GetEditorScene());
            m_Context.SetRuntimeScene(runtime);
            runtime->LaunchRuntime();
            m_Context.SetCurrentScene(runtime);
        } else {
            Scene* runtime = m_Context.GetRuntimeScene();
            if (runtime) {
                runtime->ShutdownRuntime();
                delete runtime;
            }
            m_Context.SetRuntimeScene(nullptr);
            m_Context.SetCurrentScene(m_Context.GetEditorScene());
            m_Context.GetCurrentScene()->EditorSetCamera(m_Context.GetEditorCamera());
        }

        if (m_SelectionService->HasSelection()) {
            entt::entity entity_id = m_Context.GetCurrentScene()->GetEntities().at(selected_node);
            m_SelectionService->SetSelected(Entity(entity_id, m_Context.GetCurrentScene()), true);
        }

        m_PanelManager->SetContext(m_Context.GetCurrentScene());
    }

    void HandleContentDrop(const ImGuiPayload* /*payload*/) {}

    void SaveProject() { m_ProjectService->SaveProject(); }
    void LoadProject() { m_ProjectService->LoadProject(); }
    void NewProject() { m_ProjectService->NewProject(); }

private:
    EditorContext m_Context;
    std::unique_ptr<SelectionService> m_SelectionService;
    std::unique_ptr<GizmoController> m_GizmoController;
    std::unique_ptr<ProjectService> m_ProjectService;
    PanelManager* m_PanelManager = nullptr;
};
#if 0

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
            renderer->ReleaseResourceIndex(AssetManager::Get()->GetAsset<Image>(id));
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

#endif
Ptr<Subsystem> ConstructRootSystem()
{
	return CreatePtr<EditorSubsystem>(&g_PersistentAllocator);
}