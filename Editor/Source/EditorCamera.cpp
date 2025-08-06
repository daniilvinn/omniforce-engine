#include "EditorCamera.h"

#include <Core/Input/Input.h>
#include <Core/Input/KeyCode.h>
#include <Core/Events/KeyEvents.h>

namespace Omni {

	EditorCamera::EditorCamera(float32 aspect_ratio)
	{
		Move({ 0.0f, 0.0f, -10.0f });
	}

	void EditorCamera::OnUpdate(float32 step)
	{
		float32 multiplier = 1.0f;

		if (Input::KeyPressed(KeyCode::KEY_LEFT_SHIFT))
			multiplier = 3.0f;

		if (Input::KeyPressed(KeyCode::KEY_LEFT_ALT)) {
			if (Input::ButtonPressed(ButtonCode::MOUSE_BUTTON_LEFT)) {
				if (m_FirstInteraction) {
					m_LastMousePosition = Input::MousePosition();
					m_FirstInteraction = false;
				}
				else {
					float32 x_offset = (float32)m_LastMousePosition.x - Input::MousePosition().x;
					float32 y_offset = (float32)m_LastMousePosition.y - Input::MousePosition().y;
					Rotate(-1.0f * x_offset, 0.0f, 0.0f, true);
					Rotate(0.0f, -1.0f * -y_offset, 0.0f, true);
					CalculateMatrices();
					m_LastMousePosition = Input::MousePosition();
				}
				m_InteractionIsOver = false;
			}

			else {
				if (Input::KeyPressed(KeyCode::KEY_W)) {
					Move({ 0.0f, 0.0f, 5.0f * step * multiplier});
				}
				if (Input::KeyPressed(KeyCode::KEY_A)) {
					Move({ -5.0f * step * multiplier, 0.0f, 0.0f});
				}
				if (Input::KeyPressed(KeyCode::KEY_S)) {
					Move({ 0.0f, 0.0f, -5.0f * step * multiplier });
				}
				if (Input::KeyPressed(KeyCode::KEY_D)) {
					Move({ 5.0f * step * multiplier , 0.0f, 0.0f});
				}
				if (Input::KeyPressed(KeyCode::KEY_Q)) {
					Move({ 0.0f , -5.0f * step * multiplier, 0.0f });
				}
				if (Input::KeyPressed(KeyCode::KEY_E)) {
					Move({ 0.0f , 5.0f * step * multiplier, 0.0f });
				}
				if (!m_InteractionIsOver) {
					m_FirstInteraction = true;
					m_InteractionIsOver = true;
				}
			}
		}
	}

	void EditorCamera::OnEvent(Event* e)
	{
		if (e->GetType() == Event::Type::MouseScrolled) {
			MouseScrolledEvent* mouse_scrolled_event = (MouseScrolledEvent*)e;

			float32 new_fov_in_degrees = glm::degrees(m_FieldOfView) - mouse_scrolled_event->GetAxis().y * 2;
			new_fov_in_degrees = glm::clamp(new_fov_in_degrees, 45.0f, 120.0f);
			SetFOV(glm::radians(new_fov_in_degrees));
		}
		if (e->GetType() == Event::Type::MouseMoved) {
			MouseMovedEvent* mouse_moved_event = (MouseMovedEvent*)e;
		}
	}

}