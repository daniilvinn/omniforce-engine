#pragma once

#include <Foundation/Common.h>
#include <filesystem>

namespace Omni {
    class Scene;
}

namespace Omni::EditorServices {

    class EditorContext;

    // Provides project open/save/new functionality and manages working directory.
    // This service centralizes project lifecycle logic that used to live in the editor core.
    class ProjectService {
    public:
        void Initialize(EditorContext* context);

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
    };

}



