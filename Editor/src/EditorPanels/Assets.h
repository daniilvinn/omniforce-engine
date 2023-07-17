#pragma once

#include "EditorPanel.h"

namespace Omni {

	class AssetsPanel : public EditorPanel {
	public:
		AssetsPanel(Scene* ctx);
		~AssetsPanel() {};

		void Render() override;

	};

}