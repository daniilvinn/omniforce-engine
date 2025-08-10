#pragma once

#include <Scene/Scene.h>

namespace Omni::EditorServices { class EditorContext; }

namespace Omni {

	class EditorPanel {
	public:
		EditorPanel(Scene* ctx) : m_Context(ctx), m_EditorContext(nullptr), m_IsOpen(false) {};
		virtual ~EditorPanel() {};

		void Open(bool open) { m_IsOpen = open; }
		virtual void SetContext(Scene* ctx) { m_Context = ctx; }
		virtual void SetEditorContext(EditorServices::EditorContext* ctx) { m_EditorContext = ctx; }
		virtual void Update() = 0;
	
		Scene* m_Context;
		EditorServices::EditorContext* m_EditorContext;
		bool m_IsOpen;

	};

}