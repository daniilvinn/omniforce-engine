#pragma once

#include <cursed_pch.h>

#include <Core/Input/Input.h>

namespace Omni {

	class Win64Input final : public Input {
	public:
		Win64Input() {};

		bool Impl_KeyPressed(KeyCode code, const std::string& window_tag) const override;

		bool Impl_ButtonPressed(ButtonCode code, const std::string& window_tag) const override;

		float32 Impl_MouseScrolledX(const std::string& window_tag) const override;

		float32 Impl_MouseScrolledY(const std::string& window_tag) const override;

		vec2<int32> Impl_MousePosition(const std::string& window_tag) const override;

	};

}