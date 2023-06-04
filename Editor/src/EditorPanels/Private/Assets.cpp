#include "../Assets.h"

#include <Asset/AssetManager.h>

#include <imgui.h>

#include <Renderer/Renderer.h>
#include <Renderer/UI/ImGuiRenderer.h>
#include <fstream>

#include <tinyfiledialogs/tinyfiledialogs.h>

namespace Omni {

	AssetsPanel::AssetsPanel(Scene* ctx)
		: EditorPanel(ctx)
	{
		
	}

	void AssetsPanel::Render()
	{
		auto texture_registry = AssetManager::Get()->GetTextureRegistry();

		ImGui::Begin("Assets");
		{
			ImGui::Columns(2);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
			ImGui::Text("Asset registry");
			
			ImGui::NextColumn();
			if (ImGui::Button("Browse files", { -FLT_MIN, 0.0f })) {
				const char* filters[] = {"*.png", "*.jpg", "*.jpeg"};

				char* filepath = tinyfd_openFileDialog(
					"Open file",
					(std::filesystem::current_path().string() + "\\assets").c_str(),
					3,
					filters,
					NULL,
					false
				);

				if (filepath != NULL) {
					UUID tex_uuid = AssetManager::Get()->LoadTexture(std::filesystem::path(filepath));
					auto texture_resolution = AssetManager::Get()->GetTexture(tex_uuid)->GetSpecification().extent;
				}
			}
			ImGui::Columns();
			ImGui::Separator();

			for (auto& texture : *texture_registry) {
				ImVec2 region_available = ImGui::GetContentRegionAvail();
				uvec3 image_extent = texture.second->GetSpecification().extent;
				float32 aspect_ratio = (float32)image_extent.x / (float32)image_extent.y;

				UI::RenderImage(
					texture.second, 
					SceneRenderer::GetSamplerLinear(), 
					{ region_available.x, region_available.x / aspect_ratio}
				);
				ImGui::Separator();
			}

		}
		ImGui::End();
	}

}