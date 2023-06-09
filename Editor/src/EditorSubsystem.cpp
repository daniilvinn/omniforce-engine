#include <Omniforce.h>

#include "EditorPanels/SceneHierarchy.h"
#include "EditorPanels/Properties.h"
#include "EditorPanels/Assets.h"

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

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		ImGui::Begin("Viewport");
			ImVec2 viewport_frame_size = ImGui::GetContentRegionAvail();
			UI::RenderImage(m_EditorScene->GetFinalImage(), SceneRenderer::GetSamplerNearest(), viewport_frame_size, 0, true);
		ImGui::End();
		ImGui::PopStyleVar();

		m_AssetsPanel->Render();

		// set proper viewport aspect ratio
		{
			auto camera = m_EditorScene->GetCamera();

			if (camera->GetType() == CameraProjectionType::PROJECTION_3D) {
				Shared<Camera3D> camera_3D = ShareAs<Camera3D>(camera);
				camera_3D->SetProjection(camera_3D->GetFOV(), viewport_frame_size.x / viewport_frame_size.y, 0.0f, 100.0f);
			}
			else if (camera->GetType() == CameraProjectionType::PROJECTION_2D) {
				Shared<Camera2D> camera_2D = ShareAs<Camera2D>(camera);
				float32 aspect_ratio = viewport_frame_size.x / viewport_frame_size.y;
				float32 scale = camera_2D->GetScale();
				camera_2D->SetProjection(
					-scale * aspect_ratio, 
					scale * aspect_ratio, 
					-scale, 
					scale, 
					camera_2D->GetNearClip(), 
					camera_2D->GetFarClip()
				);
			}

		}

		m_EditorScene->OnUpdate();
		ControlEditorCamera();
	}

	void OnEvent(Event* e) override
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnWindowResize));
		dispatcher.Dispatch<KeyPressedEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnKeyPressed));
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
			m_HierarchyPanel->SetContext(m_EditorScene);
		}
	};

	void ControlEditorCamera() {
		Shared<Camera> camera = m_EditorScene->GetCamera();
		Shared<Camera3D> camera_3D = ShareAs<Camera3D>(camera);
		if (Input::KeyPressed(KeyCode::KEY_A))
			camera_3D->Move({ -0.03f, 0.0f, 0.0f });
		if (Input::KeyPressed(KeyCode::KEY_D))
			camera_3D->Move({ 0.03f, 0.0f, 0.0f });
		if (Input::KeyPressed(KeyCode::KEY_W))
			camera_3D->Move({ 0.0f, 0.0f, 0.03f });
		if (Input::KeyPressed(KeyCode::KEY_S))
			camera_3D->Move({ 0.0f, 0.0f, -0.03f });
		if (Input::KeyPressed(KeyCode::KEY_Q))
			camera_3D->Rotate(-0.5f, 0.0f, 0.0f, false);
		if (Input::KeyPressed(KeyCode::KEY_E))
			camera_3D->Rotate(0.5f, 0.0f, 0.0f, false);

	}

	/*
	*	DATA
	*/
	Scene* m_EditorScene;
	Entity m_SelectedEntity;
	bool m_EntitySelected = false;

	SceneHierarchyPanel* m_HierarchyPanel;
	PropertiesPanel* m_PropertiesPanel;
	AssetsPanel* m_AssetsPanel;

	std::filesystem::path m_ProjectPath;
};

Subsystem* ConstructRootSystem()
{
	return new EditorSubsystem;
}