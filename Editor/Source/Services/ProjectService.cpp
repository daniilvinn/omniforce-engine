#include "ProjectService.h"

#include "EditorContext.h"

#include <Scene/Scene.h>
#include <Asset/AssetManager.h>
#include <Rendering/ISceneRenderer.h>
#include <Scripting/ScriptEngine.h>
#include <Filesystem/Filesystem.h>
#include <Threading/JobSystem.h>

#include "EditorCamera.h" // not directly used here
#include "../PanelManager.h"
#include "../EditorPanels/ContentBrowser.h"

#include <tinyfiledialogs.h>
#include <fstream>
#include <cstdlib>

namespace Omni::EditorServices {

    void ProjectService::Initialize(EditorContext* context, ::Omni::PanelManager* panelManager)
    {
        // Store pointer to shared editor state
        m_Context = context;
        m_PanelManager = panelManager;
    }

    void ProjectService::SaveProject()
    {
        // Resolve path; if missing, run NewProject() to ask for location
        if (m_Context->GetProjectPath().empty()) {
            NewProject();
            if (m_Context->GetProjectPath().empty()) return;
        }

        // Serialize editor scene
        nlohmann::json root = {};
        m_Context->GetEditorScene()->Serialize(root);

        // Write file to disk
        //std::filesystem::path output = m_Context->GetProjectPath() / m_Context->GetProjectFilename();
        std::filesystem::path output = m_Context->GetProjectPath();
        WriteProjectToDisk(root, output);
    }

    void ProjectService::LoadProject()
    {
        const char* filters[] = { "*.omni" };
        const char* filepath = tinyfd_openFileDialog(
            "Open project",
            std::filesystem::current_path().string().c_str(),
            1,
            filters,
            "Omniforce project files (*.omni)",
            false
        );
        if (filepath == nullptr) return;

        // Parse JSON from disk
        nlohmann::json root;
        if (!ReadProjectFromDisk(filepath, root)) return;

        // Update working directory and context fields
        std::filesystem::path p = filepath;
        m_Context->SetProjectPath(p);
        m_Context->SetProjectFilename(p.filename().string());
        p.remove_filename();
        FileSystem::SetWorkingDirectory(p);

        // Wait the device and flush assets
        Renderer::WaitDevice();

        AssetManager* assetManager = AssetManager::Get();
        auto renderer = m_Context->GetEditorScene()->GetRenderer();
        auto& registry = *assetManager->GetAssetRegistry();
        for (auto [id, asset] : registry) {
            if (asset->Type != AssetType::OMNI_IMAGE) continue;
            renderer->ReleaseResourceIndex(AssetManager::Get()->GetAsset<Image>(id));
        }
        assetManager->FullUnload();

        // Restore scene and script assemblies
        m_Context->GetEditorScene()->Deserialize(root);
        m_Context->GetEditorScene()->EditorSetCamera(m_Context->GetEditorCamera());

        // Propagate new scene context to all panels
        if (m_PanelManager) {
            // Update scene context across all panels
            m_PanelManager->SetContext(m_Context->GetEditorScene());
            // Ask Content Browser to refresh its directory listing
            if (auto* cb = m_PanelManager->GetPanelAs<ContentBrowser>("content_browser"))
                cb->Refresh();
        }

        ScriptEngine* scriptEngine = ScriptEngine::Get();
        if (scriptEngine->HasAssemblies()) scriptEngine->UnloadAssemblies();
        scriptEngine->LoadAssemblies();
    }

    void ProjectService::NewProject()
    {
        const char* filters[] = { "*.omni" };
        const char* filepath = tinyfd_saveFileDialog(
            "New project",
            std::filesystem::current_path().string().c_str(),
            1,
            filters,
            nullptr
        );
        if (filepath == nullptr) return;

        // Update paths and working directory
        std::filesystem::path p = filepath;
        m_Context->SetProjectPath(p);
        m_Context->SetProjectFilename(p.filename().string());
        p.remove_filename();
        FileSystem::SetWorkingDirectory(p);

        // Create standard project directories
        std::filesystem::create_directories(p.string() + std::string("Content"));
        std::filesystem::create_directories(p.string() + std::string("Binaries"));

        // Seed script project
        std::filesystem::copy("Resources/Scripts/ScriptsProject", p.string(), std::filesystem::copy_options::recursive);
        std::filesystem::copy("Resources/Scripting/Build/ScriptEngine.dll", p / "Binaries/ScriptEngine.dll");

        // Run the Build.bat script to build the script assemblies asynchronously to avoid blocking the engine
        std::filesystem::path buildBatPath = p / "Build.bat";
#ifdef _WIN32
        if (!std::filesystem::exists(buildBatPath))
        {
            OMNIFORCE_CLIENT_ERROR("Build script not found at: {}", buildBatPath.string());
        }
        else
        {
            // Compose a command that changes directory inside the cmd shell only (does not change process CWD)
            std::string build_dir = buildBatPath.parent_path().string();
            std::string command = "cmd /C \"cd /d \"" + build_dir + "\" && Build.bat\"";

            JobSystem::Submit([command]() {
                int exit_code = std::system(command.c_str());
                if (exit_code != 0)
                {
                    OMNIFORCE_CLIENT_ERROR("Build.bat failed with exit code: {}", exit_code);
                }
            }, { "Build scripts (Build.bat)", "Scripting", TaskPriority::Low }, JobSystem::Queue::Low);
        }
#else
        #error "Not implemented"
#endif
        SaveProject();
    }
    
    void ProjectService::WriteProjectToDisk(const nlohmann::json& rootNode, const std::filesystem::path& outputPath)
    {
        std::ofstream out(outputPath);
        out << rootNode.dump(4);
        out.close();
    }

    bool ProjectService::ReadProjectFromDisk(const char* filepath, nlohmann::json& outRootNode)
    {
        std::ifstream input(filepath);
        try {
            outRootNode = nlohmann::json::parse(input);
        }
        catch (const std::exception&) {
            OMNIFORCE_CLIENT_ERROR("Failed to load project at location: {}", filepath);
            return false;
        }
        input.close();
        return true;
    }

}



