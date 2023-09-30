#include "../SceneHierarchy.h"

#include <Scene/Component.h>
#include <Core/Input/Input.h>

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

			if (ImGui::BeginPopupContextWindow("hierarchy_create_entity_popup", ImGuiPopupFlags_MouseButtonRight |ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::MenuItem("Create object")) {
					m_SelectedNode = m_Context->CreateEntity();
					m_IsSelected = true;
				}
				ImGui::EndPopup();
			}
			ImGui::Separator();

			auto entities = m_Context->GetRegistry()->group<UUIDComponent, TagComponent>();
			for (auto& entity_id : entities) {
				Entity entity(entity_id, m_Context);
				if(!entity.GetComponent<HierarchyNodeComponent>().parent)
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
		TagComponent& tag_component = entity.GetComponent<TagComponent>();
		UUIDComponent& uc = entity.GetComponent<UUIDComponent>();
		HierarchyNodeComponent& hierarchy_node_component = entity.GetComponent<HierarchyNodeComponent>();

		ImGuiTreeNodeFlags flags = ((m_SelectedNode == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_OpenOnArrow;
		bool node_opened = ImGui::TreeNodeEx((void*)(uint64)(uint32)(entt::entity)entity, flags, tag_component.tag.c_str());

		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup(fmt::format("hierarchy_node_popup{}", uc.id).c_str());
		}

		if (ImGui::BeginPopup(fmt::format("hierarchy_node_popup{}", uc.id).c_str())) {
			if (ImGui::MenuItem("Delete")) {
				m_Context->RemoveEntity(entity);
				m_IsSelected = false;
				m_SelectedNode = { (entt::entity)0, nullptr };
			}
			if (ImGui::MenuItem("Delete with children")) {
				m_Context->RemoveEntityWithChildren(entity);
				m_IsSelected = false;
				m_SelectedNode = { (entt::entity)0, nullptr };
			}
			if (ImGui::MenuItem("Create child")) {
				Entity e = m_Context->CreateEntity();
				hierarchy_node_component.children.push_back(e.GetComponent<UUIDComponent>());
				e.GetComponent<HierarchyNodeComponent>().parent = uc.id;
				m_IsSelected = true;
				m_SelectedNode = e;
			}
			ImGui::EndPopup();
		}

		ImGuiDragDropFlags drag_and_drop_flags = ImGuiDragDropFlags_None;
		drag_and_drop_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect;

		if (ImGui::BeginDragDropSource(drag_and_drop_flags))
		{
			if (!(drag_and_drop_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
				ImGui::Text(tag_component.tag.c_str());
			ImGui::SetDragDropPayload("hierarchy_move_node_payload", &uc.id, sizeof(uc.id));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("hierarchy_move_node_payload", target_flags);
			if (payload)
			{
				UUID id(*(uint64*)payload->Data);
				Entity moved_entity = m_Context->GetEntity(id);

				HierarchyNodeComponent& moded_entity_hier = moved_entity.GetComponent<HierarchyNodeComponent>();

				if (moved_entity.HasInHierarchy(entity))
					return;

				if (moved_entity.GetParent()) {
					Entity previous_parent = moved_entity.GetParent();
					HierarchyNodeComponent& prev_parent_hierarchy = previous_parent.GetComponent<HierarchyNodeComponent>();
					for (auto i = prev_parent_hierarchy.children.begin(); i != prev_parent_hierarchy.children.end(); i++) {
						if (*i == id) {
							prev_parent_hierarchy.children.erase(i);
							break;
						}
					}
				}

				hierarchy_node_component.children.push_back(id);

				Entity child = m_Context->GetEntity(id);
				child.GetComponent<HierarchyNodeComponent>().parent = uc.id;
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemClicked()) {
			if (Input::ButtonPressed(ButtonCode::MOUSE_BUTTON_LEFT)) {
				m_SelectedNode = entity;
				m_IsSelected = true;
			}
		}
		if (node_opened) {
			for (auto& child : hierarchy_node_component.children) {
				RenderHierarchyNode(m_Context->GetEntity(child));
			}
			ImGui::TreePop();
		}
	}

}