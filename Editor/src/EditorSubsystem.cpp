#include <Omniforce.h>

#include "EditorPanels/SceneHierarchy.h"
#include "EditorPanels/Properties.h"
#include "EditorPanels/Assets.h"

#include "EditorCamera.h"

#include <filesystem>
#include <fstream>

#include <tinyfiledialogs/tinyfiledialogs.h>
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
			ImGui::EndPopup();
		}
		ImGui::EndMainMenuBar();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		//Render and process scene hierarchy panel
		ImGui::BeginDisabled(m_InRuntime);
		m_HierarchyPanel->Render();
		if (m_EntitySelected = m_HierarchyPanel->IsNodeSelected()) 
		{
			m_SelectedEntity = m_HierarchyPanel->GetSelectedNode();
		}

		// Properties panel
		m_PropertiesPanel->SetEntity(m_SelectedEntity, m_HierarchyPanel->IsNodeSelected());
		m_PropertiesPanel->Render();

		// Asset panel
		m_AssetsPanel->Render();

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
				if (m_RuntimeScene)
					delete m_RuntimeScene;
				m_RuntimeScene = new Scene(m_EditorScene); 
				m_RuntimeScene->LaunchRuntime();
				m_CurrentScene = m_RuntimeScene;
			}
			else { 
				m_RuntimeScene->ShutdownRuntime();
				m_CurrentScene = m_EditorScene;
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
			UI::RenderImage(m_EditorScene->GetFinalImage(), SceneRenderer::GetSamplerNearest(), viewport_frame_size, 0, true);
			m_EditorCamera->SetAspectRatio(viewport_frame_size.x / viewport_frame_size.y);
			
			if (m_InRuntime) {
				if(m_RuntimeScene->GetCamera() != nullptr)
					m_RuntimeScene->GetCamera()->SetAspectRatio(viewport_frame_size.x / viewport_frame_size.y);
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
			m_EditorCamera->OnUpdate();
	}

	void Launch() override
	{
		JobSystem::Get()->Wait();

		m_EditorScene = new Scene(SceneType::SCENE_TYPE_3D);

		m_HierarchyPanel = new SceneHierarchyPanel(m_EditorScene);
		m_PropertiesPanel = new PropertiesPanel(m_EditorScene);
		m_AssetsPanel = new AssetsPanel(m_EditorScene);

		m_ProjectPath = "";

		m_EditorCamera = std::make_shared<EditorCamera>(16.0 / 9.0, 20.0f);
		m_EditorScene->EditorSetCamera(ShareAs<Camera>(m_EditorCamera));
		m_CurrentScene = m_EditorScene;

		ImGuizmo::SetOrthographic(true);
		
		m_ProjectPath = "resources/SandboxProject";
		m_ProjectFilename = "Sandbox.omni";
		FileSystem::SetWorkingDirectory(m_ProjectPath);
	}

	void Destroy() override
	{
		Renderer::WaitDevice();
		m_EditorScene->Destroy();
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
					m_CurrentOperation = ImGuizmo::OPERATION::TRANSLATE_X | ImGuizmo::OPERATION::TRANSLATE_Y;
				if (Input::KeyPressed(KeyCode::KEY_E))
					m_CurrentOperation = ImGuizmo::OPERATION::ROTATE_Z;
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

		char* filepath = tinyfd_openFileDialog(
			"Open project",
			std::filesystem::current_path().string().c_str(),
			1,
			filters,
			"Omniforce project files (*.omni)",
			false
		);

		if (filepath == NULL)
			return;

		m_ProjectPath = filepath;
		m_ProjectFilename = m_ProjectPath.filename().string();
		m_ProjectPath.remove_filename();

		FileSystem::SetWorkingDirectory(m_ProjectPath);
			
		Renderer::WaitDevice();

		// unloading textures from memory and releasing their indices
		AssetManager* asset_manager = AssetManager::Get();
		Shared<SceneRenderer> renderer = m_EditorScene->GetRenderer();
		auto texture_registry = *asset_manager->GetTextureRegistry();
		for (auto& [id, texture] : texture_registry) {
			renderer->ReleaseTextureIndex(texture);
			texture->Destroy();
		}
		asset_manager->FullUnload();

		std::ifstream input(filepath);
		nlohmann::json root_node = nlohmann::json::parse(input);
		input.close();

		m_EditorScene->Deserialize(root_node);
		m_EditorScene->EditorSetCamera(m_EditorCamera);
		m_HierarchyPanel->SetContext(m_EditorScene);
		m_HierarchyPanel->SetSelectedNode({ (entt::entity)0, m_CurrentScene }, false);

		ScriptEngine* script_engine = ScriptEngine::Get();
		if (script_engine->HasAssemblies())
			script_engine->UnloadAssemblies();
		script_engine->LoadAssemblies();
	};

	void NewProject() {
		const char* filters[] = { "*.omni" };

		char* filepath = tinyfd_saveFileDialog(
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

		auto textures_dir = m_ProjectPath.string() + "/assets/textures";
		auto scripts_dir = m_ProjectPath.string() + "/assets/scripts";
		auto audio_dir = m_ProjectPath.string() + "/assets/audio";

		std::filesystem::create_directories(textures_dir);
		std::filesystem::create_directories(scripts_dir);
		std::filesystem::create_directories(audio_dir);

		std::filesystem::copy("resources/scripting/ScriptsProject", m_ProjectPath.string() + "/assets/scripts", std::filesystem::copy_options::recursive);
		std::filesystem::copy("resources/scripting/bin/Debug/ScriptEngine.dll", m_ProjectPath.string() + "/assets/scripts/ScriptEngine.dll");

		if(m_ProjectPath.string().length())
			SaveProject();
	}

	void RenderGizmos() {
		if (m_EntitySelected && m_CurrentOperation) {
			TRSComponent trs = m_SelectedEntity.GetWorldTransform();

			glm::mat4 model = Utils::ComposeMatrix(trs.translation, trs.rotation, trs.scale);
			glm::mat4 view = m_EditorCamera->GetViewMatrix();
			glm::mat4 proj = m_EditorCamera->GetProjectionMatrix();

			ImGuizmo::SetOrthographic(true);
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
				TRSComponent& trs = m_SelectedEntity.GetComponent<TRSComponent>();
				Utils::DecomposeMatrix(model, &trs.translation, &trs.rotation, &trs.scale);

				if (m_SelectedEntity.GetComponent<HierarchyNodeComponent>().parent.Valid()) {
					Entity parent_entity = m_SelectedEntity.GetParent();

					TRSComponent parent_trs = parent_entity.GetWorldTransform();
					trs.translation -= parent_trs.translation;
					trs.rotation -= parent_trs.scale;
				}

			}
			
		}	
	}

	/*
	*	DATA
	*/
	Scene* m_EditorScene;
	Scene* m_RuntimeScene = nullptr;
	Scene* m_CurrentScene;
	Shared<EditorCamera> m_EditorCamera;
	Entity m_SelectedEntity;
	bool m_EntitySelected = false;
	bool m_ViewportFocused = false;
	bool m_InRuntime = false;

	SceneHierarchyPanel* m_HierarchyPanel;
	PropertiesPanel* m_PropertiesPanel;
	AssetsPanel* m_AssetsPanel;

	std::filesystem::path m_ProjectPath;
	std::string m_ProjectFilename;
	ImGuizmo::OPERATION m_CurrentOperation = (ImGuizmo::OPERATION)0;

	fvec2 m_ViewportBounds[2];

};

Subsystem* ConstructRootSystem()
{
	return new EditorSubsystem;
}