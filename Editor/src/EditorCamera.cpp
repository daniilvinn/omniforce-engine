#include "EditorCamera.h"

#include <Core/Input/Input.h>
#include <Core/Input/KeyCode.h>
#include <Core/Events/KeyEvents.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Omni {

	EditorCamera::EditorCamera(float32 aspect_ratio, float32 scale)
	{
		m_ZNear = 0.0f;
		m_ZFar = 1.0f;
		m_AspectRatio = aspect_ratio;
		SetScale(scale);
	}

	void EditorCamera::OnUpdate()
	{
		if (Input::KeyPressed(KeyCode::KEY_LEFT_CONTROL)) {
			if (Input::ButtonPressed(ButtonCode::MOUSE_BUTTON_LEFT)) {
				if (m_FirstInteraction) {
					m_LastMousePosition = Input::MousePosition();
					m_FirstInteraction = false;
				}
				else {
					float32 x_offset = (float32)m_LastMousePosition.x - Input::MousePosition().x;
					float32 y_offset = (float32)m_LastMousePosition.y - Input::MousePosition().y;
					m_Position.x += (x_offset / 90.0) * (m_Scale / 5.0f);
					m_Position.y -= (y_offset / 90.0) * (m_Scale / 5.0f);
					CalculateMatrices();
					m_LastMousePosition = Input::MousePosition();
				}
				m_InteractionIsOver = false;
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
			SetScale(m_Scale - mouse_scrolled_event->GetAxis().y);
		}
	}

}