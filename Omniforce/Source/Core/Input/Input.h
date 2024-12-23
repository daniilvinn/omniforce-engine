#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Core/Input/KeyCode.h>

#include <string>

namespace Omni 
{

	class OMNIFORCE_API Input 
	{
	public:
		static void Init();
		static void Shutdown();

		static bool			KeyPressed(KeyCode code, const std::string& window_tag = "main") { return s_Instance->Impl_KeyPressed(code, window_tag); };
		static bool			ButtonPressed(ButtonCode code, const std::string& window_tag = "main") { return s_Instance->Impl_ButtonPressed(code, window_tag); };
		static float32		MouseScrolledX(const std::string& window_tag = "main") { return s_Instance->Impl_MouseScrolledX(window_tag); }
		static float32		MouseScrolledY(const std::string& window_tag = "main") { return s_Instance->Impl_MouseScrolledY(window_tag); }
		static ivec2		MousePosition(const std::string& window_tag = "main") { return s_Instance->Impl_MousePosition(window_tag); }

		static void LockAndHideMouse(const std::string& window_tag = "main") { return s_Instance->Impl_LockAndHideMouse(window_tag); }
		static void ReleaseAndShowMouse(const std::string& window_tag = "main") { return s_Instance->Impl_ReleaseAndShowMouse(window_tag); }

		static float64 Time() { return s_Instance->Impl_Time(); }

	protected:
		Input() {};

		virtual bool		Impl_KeyPressed(KeyCode code, const std::string& window_tag) const = 0;
		virtual bool		Impl_ButtonPressed(ButtonCode code, const std::string& window_tag) const = 0;
		virtual float32		Impl_MouseScrolledX(const std::string& window_tag) const = 0;
		virtual float32		Impl_MouseScrolledY(const std::string& window_tag) const = 0;
		virtual ivec2		Impl_MousePosition(const std::string& window_tag) const = 0;
		virtual void		Impl_LockAndHideMouse(const std::string& window_tag) = 0;
		virtual void		Impl_ReleaseAndShowMouse(const std::string& window_tag) = 0;
		virtual float64		Impl_Time() = 0;

	protected:
		static Input* s_Instance;
	};

}