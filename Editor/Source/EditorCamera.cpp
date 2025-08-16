#include "EditorCamera.h"

#include <Core/Input/Input.h>
#include <Core/Input/KeyCode.h>
#include <Core/Events/KeyEvents.h>
#include <cmath>

namespace Omni {

	EditorCamera::EditorCamera(float32 aspect_ratio)
	{
		Move({ 0.0f, 0.0f, -10.0f });
	}

	EditorCamera::~EditorCamera()
	{
		// Ensure mouse is released if camera is destroyed while dragging
		if (m_IsRightMouseDragging) {
			Input::ReleaseAndShowMouse();
		}
	}

	void EditorCamera::OnUpdate(float32 step)
	{
		// Speed multipliers
		float32 speed_multiplier = 1.0f;
		
		if (Input::KeyPressed(KeyCode::KEY_LEFT_SHIFT))
			speed_multiplier = 3.0f;

		// Handle mouse capture state changes
		bool right_mouse_pressed = Input::ButtonPressed(ButtonCode::MOUSE_BUTTON_RIGHT);

		// Start right mouse drag
		if (right_mouse_pressed && !m_IsRightMouseDragging) {
			m_IsRightMouseDragging = true;
			m_LastMousePosition = Input::MousePosition();
			Input::LockAndHideMouse();
		}
		// End right mouse drag  
		else if (!right_mouse_pressed && m_IsRightMouseDragging) {
			m_IsRightMouseDragging = false;
			Input::ReleaseAndShowMouse();
		}

		// Handle camera rotation
		if (m_IsRightMouseDragging) {
			ivec2 current_pos = Input::MousePosition();
			float32 x_delta = -(float32)(current_pos.x - m_LastMousePosition.x);
			float32 y_delta = (float32)(current_pos.y - m_LastMousePosition.y);
			
			// Apply rotation sensitivity
			float32 rotation_sensitivity = 0.15f;
			x_delta *= rotation_sensitivity;
			y_delta *= rotation_sensitivity;
			
			if (fabsf(x_delta) > 0.01f || fabsf(y_delta) > 0.01f) {
				Rotate(-x_delta, 0.0f, 0.0f, true);
				Rotate(0.0f, -y_delta, 0.0f, true);
				CalculateMatrices();
			}
			
			m_LastMousePosition = current_pos;
		}

		// Handle movement during right mouse drag
		if (m_IsRightMouseDragging) {
			if (Input::KeyPressed(KeyCode::KEY_W)) {
				Move({ 0.0f, 0.0f, 8.0f * step * speed_multiplier});
			}
			if (Input::KeyPressed(KeyCode::KEY_A)) {
				Move({ -8.0f * step * speed_multiplier, 0.0f, 0.0f});
			}
			if (Input::KeyPressed(KeyCode::KEY_S)) {
				Move({ 0.0f, 0.0f, -8.0f * step * speed_multiplier });
			}
			if (Input::KeyPressed(KeyCode::KEY_D)) {
				Move({ 8.0f * step * speed_multiplier , 0.0f, 0.0f});
			}
			if (Input::KeyPressed(KeyCode::KEY_Q)) {
				Move({ 0.0f , -8.0f * step * speed_multiplier, 0.0f });
			}
			if (Input::KeyPressed(KeyCode::KEY_E)) {
				Move({ 0.0f , 8.0f * step * speed_multiplier, 0.0f });
			}
		}
	}

	void EditorCamera::OnEvent(Event* e)
	{
		if (e->GetType() == Event::Type::MouseScrolled) {
			MouseScrolledEvent* mouse_scrolled_event = (MouseScrolledEvent*)e;
			
			// Mouse scroll for forward/backward movement (UE5 style)
			float32 scroll_speed = 2.0f;
			Move({ 0.0f, 0.0f, mouse_scrolled_event->GetAxis().y * scroll_speed });
		}
	}

}