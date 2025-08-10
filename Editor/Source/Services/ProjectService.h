#pragma once

#include <Foundation/Common.h>
#include <filesystem>

namespace Omni {
    class Scene;
}

namespace Omni { class PanelManager; }
namespace Omni::EditorServices {

    class EditorContext;

    // Provides project open/save/new functionality and manages working directory.
    // This service centralizes project lifecycle logic that used to live in the editor core.
    class ProjectService {
    public:
        // Initialize service with shared editor state and panel manager for context propagation
        void Initialize(EditorContext* context, ::Omni::PanelManager* panelManager);

        // File menu commands
        void SaveProject();
        void LoadProject();
        void NewProject();

    private:
        // Helper functions
        void WriteProjectToDisk(const nlohmann::json& rootNode, const std::filesystem::path& outputPath);
        bool ReadProjectFromDisk(const char* filepath, nlohmann::json& outRootNode);

    private:
        EditorContext* m_Context = nullptr;
        ::Omni::PanelManager* m_PanelManager = nullptr;
    };

}



