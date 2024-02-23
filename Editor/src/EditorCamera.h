#pragma once

#include <Scene/Camera.h>
#include <Core/Events/Event.h>
#include <Core/Events/MouseEvents.h>

#include <glm/glm.hpp>

namespace Omni {

	class EditorCamera : public Camera3D
	{
	public:
		EditorCamera() = default;
		EditorCamera(float32 aspect_ratio);

		void OnUpdate(float32 step);
		void OnEvent(Event* e);

	private:
		ivec2 m_LastMousePosition = { 0, 0 };
		bool m_FirstInteraction = true;
		bool m_InteractionIsOver = true;

	};
}