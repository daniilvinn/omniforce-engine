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

		m_PropertiesPanel->SetEntity(m_SelectedEntity, m_EntitySelected);
		m_PropertiesPanel->Render();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		ImGui::Begin("Viewport");
			ImVec2 viewport_frame_size = ImGui::GetContentRegionAvail();
			UI::RenderImage(m_EditorScene->GetFinalImage(), SceneRenderer::GetSamplerNearest(), viewport_frame_size, 0, true);
		ImGui::End();
		ImGui::PopStyleVar();

#if 0
		ImGui::Begin("Debug", &debug_opened);
		{
			if (ImGui::Button("Generate dump")) OMNIFORCE_WRITE_LOGS_TO_FILE();
		}
		ImGui::End();
#endif

		m_AssetsPanel->Render();

		auto camera = m_EditorScene->GetCamera();
		Shared<Camera3D> camera_3D = ShareAs<Camera3D>(camera);

		camera_3D->SetProjection(glm::radians(90.0f), viewport_frame_size.x / viewport_frame_size.y, 0.0f, 100.0f);

		m_EditorScene->OnUpdate();
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

		m_EditorScene = new Scene(SceneType::SCENE_TYPE_2D);

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
		root_node.emplace("Textures", nlohmann::json::object());
		root_node.emplace("Entities", nlohmann::json::object());

		nlohmann::json& texture_node = root_node["Textures"];

		auto tex_registry = *AssetManager::Get()->GetTextureRegistry();

		for (auto& [id, texture] : tex_registry) {
			texture_node.emplace(std::to_string(texture->GetId()), texture->GetSpecification().path.string().erase(0, std::filesystem::current_path().string().size() + 1));
		}

		nlohmann::json& entities_node = root_node["Entities"];

		auto& entities = m_EditorScene->GetEntities();
		for (auto& entity : entities) {
			uint64 tag = entity.GetComponent<UUIDComponent>().id.Get();
			nlohmann::json entity_node;

			entity.Serialize(entity_node);

			entities_node.emplace(std::to_string(tag), entity_node);

		}

		root_node.emplace("Entities", entities_node);

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
			nlohmann::json project = nlohmann::json::parse(input);
			input.close();

			nlohmann::json textures = project["Textures"];

			for (auto i = textures.begin(); i != textures.end(); i++) {
				std::string texture_path = std::filesystem::current_path().string() + "\\" + i.value().get<std::string>();

				Omni::UUID id = AssetManager::Get()->LoadTexture(texture_path, std::stoull(i.key()));

				Shared<Image> texture = AssetManager::Get()->GetTexture(id);
				m_EditorScene->GetRenderer()->AcquireTextureIndex(texture, SamplerFilteringMode::NEAREST);
			}
			
			nlohmann::json& entities_node = project["Entities"];
			for (auto i = entities_node.begin(); i != entities_node.end(); i++) {
				Entity entity = m_EditorScene->CreateEntity(std::stoull(i.key()));
				entity.Deserialize(i.value());
			}

		}
	};

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