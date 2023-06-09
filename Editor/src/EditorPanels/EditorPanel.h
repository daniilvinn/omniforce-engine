#pragma once

#include <Scene/Scene.h>

namespace Omni {

	class EditorPanel {
	protected:
		EditorPanel(Scene* ctx) : m_Context(ctx), m_IsOpen(false) {};
		virtual ~EditorPanel() {};

		virtual void SetContext(Scene* ctx) { m_Context = ctx; };
		virtual void Render() = 0;
	
		Scene* m_Context;
		bool m_IsOpen;

	};

}