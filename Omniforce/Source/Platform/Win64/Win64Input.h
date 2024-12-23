#pragma once

#include <Foundation/Types.h>
#include <Core/Input/Input.h>

namespace Omni {

	class Win64Input final : public Input {
	public:
		Win64Input() {};

		bool Impl_KeyPressed(KeyCode code, const std::string& window_tag) const override;

		bool Impl_ButtonPressed(ButtonCode code, const std::string& window_tag) const override;

		float32 Impl_MouseScrolledX(const std::string& window_tag) const override;

		float32 Impl_MouseScrolledY(const std::string& window_tag) const override;

		ivec2 Impl_MousePosition(const std::string& window_tag) const override;

		void Impl_LockAndHideMouse(const std::string& window_tag) override;

		void Impl_ReleaseAndShowMouse(const std::string& window_tag) override;

		float64 Impl_Time() override;

	};

}