#include <Omniforce.h>

#include "EditorPanels/SceneHierarchy.h"
#include "EditorPanels/Properties.h"
#include "EditorPanels/Assets.h"

#include "EditorCamera.h"

#include <filesystem>
#include <fstream>

#include <tinyfiledialogs/tinyfiledialogs.h>

using namespace Omni;

class EditorSubsystem : public Subsystem
{
public:
	~EditorSubsystem() override
	{
		Destroy();
	}

	void OnUpdate() override
	{
		ImGui::BeginMainMenuBar();
		if (ImGui::MenuItem("File")) {
			ImGui::OpenPopup("menu_bar_file");
		};
		if (ImGui::BeginPopup("menu_bar_file")) {
			if (ImGui::MenuItem("Open project")) {
				LoadProject();
			};
			ImGui::EndPopup();
		};
		ImGui::EndMainMenuBar();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		m_HierarchyPanel->Render();

		if (m_HierarchyPanel->IsNodeSelected()) {
			m_SelectedEntity = m_HierarchyPanel->GetSelectedNode();
			m_EntitySelected = true;
		}

		m_PropertiesPanel->SetEntity(m_SelectedEntity, m_HierarchyPanel->IsNodeSelected());
		m_PropertiesPanel->Render();

		ImGui::Begin("##Toolbar", nullptr, 
			ImGuiWindowFlags_NoDecoration | 
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | 
			ImGuiWindowFlags_NoTitleBar
		);
		if (ImGui::Button(m_InRuntime ? "Stop" : "Play")) {
			m_InRuntime = !m_InRuntime;
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
			m_HierarchyPanel->SetContext(m_CurrentScene);
			m_HierarchyPanel->SetSelectedNode(m_SelectedEntity, m_HierarchyPanel->IsNodeSelected());
			m_PropertiesPanel->SetContext(m_CurrentScene);
			m_PropertiesPanel->SetEntity(m_SelectedEntity, m_HierarchyPanel->IsNodeSelected());
		};
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		ImGui::Begin("Viewport");
			m_ViewportFocused = ImGui::IsWindowFocused();
			ImVec2 viewport_frame_size = ImGui::GetContentRegionAvail();
			UI::RenderImage(m_EditorScene->GetFinalImage(), SceneRenderer::GetSamplerNearest(), viewport_frame_size, 0, true);
			m_EditorCamera->SetViewportSize(viewport_frame_size.x, viewport_frame_size.y);
			
			if (m_InRuntime) {
				m_RuntimeScene->GetCamera()->SetAspectRatio(viewport_frame_size.x / viewport_frame_size.y);
			}

		ImGui::End();
		ImGui::PopStyleVar();

		m_AssetsPanel->Render();

		m_CurrentScene->OnUpdate();

		if(m_ViewportFocused && !m_InRuntime)
			m_EditorCamera->OnUpdate();
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
			}
		}
		return false;
	}

	void Destroy() override
	{
		Renderer::WaitDevice();
		m_EditorScene->Destroy();
	}

	void Launch() override
	{
		JobSystem::Get()->Wait();

		m_EditorScene = new Scene(SceneType::SCENE_TYPE_3D);

		m_HierarchyPanel = new SceneHierarchyPanel(m_EditorScene);
		m_PropertiesPanel = new PropertiesPanel(m_EditorScene);
		m_AssetsPanel = new AssetsPanel(m_EditorScene);

		m_ProjectPath = "";

		m_EditorCamera = std::make_shared<EditorCamera>(45.0f, 1.778, 0.01, 1000.0f);
		m_EditorScene->EditorSetCamera(ShareAs<Camera>(m_EditorCamera));
		m_CurrentScene = m_EditorScene;
	}

	void SaveProject() {
		if (m_ProjectPath.empty()) {
			const char* filters[] = { "*.json" };

			char* filepath = tinyfd_saveFileDialog(
				"Save project",
				std::filesystem::current_path().string().c_str(),
				1,
				filters,
				nullptr
			);

			if (filepath == NULL)
				return;

			m_ProjectPath = filepath;

		}

		nlohmann::json root_node = {};
		m_EditorScene->Serialize(root_node);

		std::ofstream out_stream(m_ProjectPath);
		out_stream << root_node.dump(4);
		out_stream.close();
		
	};

	void LoadProject() 
	{
		const char* filters[] = { "*.json" };

		char* filepath = tinyfd_openFileDialog(
			"Open project",
			std::filesystem::current_path().string().c_str(),
			1,
			filters,
			"Omniforce project files (*.json)",
			false
		);
		if (filepath != NULL) {
			m_ProjectPath = filepath;

			std::ifstream input(filepath);
			nlohmann::json root_node = nlohmann::json::parse(input);
			input.close();

			m_EditorScene->Deserialize(root_node);
			m_EditorScene->EditorSetCamera(m_EditorCamera);
			m_HierarchyPanel->SetContext(m_EditorScene);
		}
	};

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
};

Subsystem* ConstructRootSystem()
{
	return new EditorSubsystem;
}