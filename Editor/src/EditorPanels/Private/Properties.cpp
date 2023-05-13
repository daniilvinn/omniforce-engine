#include "../Properties.h"

#include <Scene/Component.h>

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

namespace Omni {

	void PropertiesPanel::Render()
	{
		ImGui::Begin("Properties", &m_IsOpen);
		if (m_IsOpen) 
		{
			if (m_Selected) {
				std::string& tag = m_Entity.GetComponent<TagComponent>().tag;
				ImGui::Text("Entity name: ");

				const uint32 STR_BUFFER_SIZE = 256;
				char str_buf[STR_BUFFER_SIZE];
				strcpy_s(str_buf, STR_BUFFER_SIZE, tag.data());

				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::InputText("##", str_buf, STR_BUFFER_SIZE);
				strcpy_s(tag.data(), tag.capacity(), str_buf);
				
				glm::mat4& transform = m_Entity.GetComponent<TransformComponent>().matrix;
				auto& trs_component = m_Entity.GetComponent<TRSComponent>();

				ImGui::Text("Translation");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-FLT_MIN);
				if(ImGui::DragFloat3("##floatT", glm::value_ptr(trs_component.translation), 0.01f))
					RecalculateTransform();
				ImGui::Separator();

				ImGui::Text("Rotation");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-FLT_MIN);
				if(ImGui::DragFloat3("##floatR", glm::value_ptr(trs_component.rotation), 0.1f))
					RecalculateTransform();
				ImGui::Separator();

				ImGui::Text("Scale");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-FLT_MIN);
				if(ImGui::DragFloat3("##floatS", glm::value_ptr(trs_component.scale), 0.01f))
					RecalculateTransform();
				ImGui::Separator();
			}
		}
		ImGui::End();
	}

	void PropertiesPanel::RecalculateTransform()
	{
		glm::mat4& matrix = m_Entity.GetComponent<TransformComponent>().matrix;
		auto& trs_component = m_Entity.GetComponent<TRSComponent>();

		matrix = glm::translate(glm::mat4(1.0f), trs_component.translation) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(trs_component.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
			glm::scale(glm::mat4(1.0f), trs_component.scale);

	}

}