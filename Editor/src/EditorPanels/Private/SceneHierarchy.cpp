#include "../SceneHierarchy.h"

#include <Scene/Component.h>

#include <imgui.h>
#include <entt/entt.hpp>

namespace Omni {

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{

	}

	void SceneHierarchyPanel::Render()
	{
		if (m_IsOpen) {
			ImGui::Begin("Scene Hierarchy", &m_IsOpen);
			ImGui::Text("Right-click to create object");

			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Create object")) {
					m_SelectedNode = m_Context->CreateEntity();
					m_IsSelected = true;
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();
			auto& entities = m_Context->GetEntities();
			for(auto& [uuid, entity_id] : entities) {
				Entity entity(entity_id, m_Context);
				RenderHierarchyNode(entity);
			};
			ImGui::End();
		}
	}

	void SceneHierarchyPanel::SetContext(Scene* ctx)
	{
		m_Context = ctx;
		m_SelectedNode = { (entt::entity)0, ctx };
	}

	void SceneHierarchyPanel::RenderHierarchyNode(Entity entity)
	{
		auto& tag_component = entity.GetComponent<TagComponent>();
		ImGuiTreeNodeFlags flags = ((m_SelectedNode == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_OpenOnArrow;
		bool node_opened = ImGui::TreeNodeEx((void*)(uint64)(uint32)(entt::entity)entity, flags, tag_component.tag.c_str());
		if (ImGui::IsItemClicked()) {
			m_SelectedNode = entity;
			m_IsSelected = true;
		}
		if (node_opened) {
			ImGui::TreePop();
		}
	}

}