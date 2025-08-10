#pragma once

#include <Foundation/Common.h>
#include <Scene/Entity.h>

namespace Omni {

    class Scene;
    class EditorCamera;
}

namespace Omni::EditorServices {

    class SelectionService;

    // Central shared state for the editor. Panels and services should depend on this
    // instead of reaching into the editor core directly.
    class EditorContext {
    public:
        EditorContext() = default;

        // Scene state
        void SetEditorScene(Scene* scene) { m_EditorScene = scene; }
        Scene* GetEditorScene() const { return m_EditorScene; }

        void SetRuntimeScene(Scene* scene) { m_RuntimeScene = scene; }
        Scene* GetRuntimeScene() const { return m_RuntimeScene; }

        void SetCurrentScene(Scene* scene) { m_CurrentScene = scene; }
        Scene* GetCurrentScene() const { return m_CurrentScene; }

        void SetEditorCamera(Ref<EditorCamera> camera) { m_EditorCamera = camera; }
        Ref<EditorCamera> GetEditorCamera() const { return m_EditorCamera; }

        // Run state
        void SetInRuntime(bool value) { m_InRuntime = value; }
        bool IsInRuntime() const { return m_InRuntime; }

        void SetViewportFocused(bool value) { m_ViewportFocused = value; }
        bool IsViewportFocused() const { return m_ViewportFocused; }

        void SetViewportBounds(const fvec2 (&bounds)[2]) { m_ViewportBounds[0] = bounds[0]; m_ViewportBounds[1] = bounds[1]; }
        const fvec2* GetViewportBounds() const { return m_ViewportBounds; }

        // Debug toggles
        bool& VisualizeColliders() { return m_VisualizeColliders; }
        bool& VisualizeCullBounds() { return m_VisualizeCullBounds; }
        bool& SceneDebugViewEnabled() { return m_SceneDebugViewEnabled; }

        // Project
        void SetProjectPath(const std::filesystem::path& path) { m_ProjectPath = path; }
        const std::filesystem::path& GetProjectPath() const { return m_ProjectPath; }

        void SetProjectFilename(const std::string& name) { m_ProjectFilename = name; }
        const std::string& GetProjectFilename() const { return m_ProjectFilename; }

        // Services
        void SetSelectionService(SelectionService* service) { m_SelectionService = service; }
        SelectionService* GetSelectionService() const { return m_SelectionService; }

    private:
        // Scenes
        Scene* m_EditorScene = nullptr;
        Scene* m_RuntimeScene = nullptr;
        Scene* m_CurrentScene = nullptr;
        Ref<EditorCamera> m_EditorCamera;

        // Viewport
        fvec2 m_ViewportBounds[2] = {};
        bool m_ViewportFocused = false;

        // Run state
        bool m_InRuntime = false;

        // Debug toggles
        bool m_VisualizeColliders = false;
        bool m_VisualizeCullBounds = false;
        bool m_SceneDebugViewEnabled = false;

        // Project
        std::filesystem::path m_ProjectPath;
        std::string m_ProjectFilename;

        // Services
        SelectionService* m_SelectionService = nullptr;
    };

}


