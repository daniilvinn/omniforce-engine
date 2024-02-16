#include "EditorCamera.h"

#include <Core/Input/Input.h>
#include <Core/Input/KeyCode.h>
#include <Core/Events/KeyEvents.h>

namespace Omni {

	EditorCamera::EditorCamera(float32 aspect_ratio)
	{
	}

	void EditorCamera::OnUpdate()
	{
		if (Input::KeyPressed(KeyCode::KEY_LEFT_ALT)) {
			if (Input::ButtonPressed(ButtonCode::MOUSE_BUTTON_LEFT)) {
				if (m_FirstInteraction) {
					m_LastMousePosition = Input::MousePosition();
					m_FirstInteraction = false;
				}
				else {
					float32 x_offset = (float32)m_LastMousePosition.x - Input::MousePosition().x;
					float32 y_offset = (float32)m_LastMousePosition.y - Input::MousePosition().y;
					m_Position.x += (x_offset / 90.0) * (20.0f / 5.0f);
					m_Position.y -= (y_offset / 90.0) * (20.0f / 5.0f);
					CalculateMatrices();
					m_LastMousePosition = Input::MousePosition();
				}
				m_InteractionIsOver = false;
			}
			else if (Input::KeyPressed(KeyCode::KEY_W)) {
				Move({ 0.0f, 0.0f, 0.01f });
			}
			else if (Input::KeyPressed(KeyCode::KEY_A)) {
				Move({ -0.01f, 0.0f, 0.0f });
			}
			else if (Input::KeyPressed(KeyCode::KEY_S)) {
				Move({ 0.0f, 0.0f, -0.01f });
			}
			else if (Input::KeyPressed(KeyCode::KEY_D)) {
				Move({ 0.01f, 0.0f, 0.0f });
			}
			else {
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
	}

}