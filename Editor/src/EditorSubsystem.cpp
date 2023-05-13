#include <Omniforce.h>

#include "EditorPanels/SceneHierarchy.h"
#include "EditorPanels/Properties.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

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
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		ImGui::ShowDemoWindow();

		m_HierarchyPanel->Render();

		if (m_HierarchyPanel->IsNodeSelected()) {
			m_SelectedEntity = m_HierarchyPanel->GetSelectedNode();
			m_EntitySelected = true;
		}

		m_PropertiesPanel->SetEntity(m_SelectedEntity, m_EntitySelected);
		m_PropertiesPanel->Render();

		ImGui::Begin("Viewport");
			UI::RenderImage(m_ViewportPanel, SceneRenderer::GetSamplerNearest(), ImGui::GetContentRegionAvail(), 0);
		ImGui::End();
	}

	void OnEvent(Event* e) override
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnWindowResize));
	}

	bool OnWindowResize(WindowResizeEvent* e) {
		return false;
	}

	void Destroy() override
	{
		Renderer::WaitDevice();
		m_ViewportPanel->Destroy();
		m_Scene->Destroy();
	}

	void Launch() override
	{
		JobSystem::Get()->Wait();

		m_Scene = new Scene(SceneType::SCENE_TYPE_2D);

		m_HierarchyPanel = new SceneHierarchyPanel(m_Scene);
		m_PropertiesPanel = new PropertiesPanel(m_Scene);

		ImageSpecification image_spec = ImageSpecification::Default();
		image_spec.path = "assets/textures/sprite.png";
		m_ViewportPanel = Image::Create(image_spec);
	}

	/*
	*	DATA
	*/
	Scene* m_Scene;
	Entity m_SelectedEntity;
	bool m_EntitySelected = false;

	SceneHierarchyPanel* m_HierarchyPanel;
	PropertiesPanel* m_PropertiesPanel;

	Shared<Image> m_ViewportPanel;
};

Subsystem* ConstructRootSystem()
{
	return new EditorSubsystem;
}