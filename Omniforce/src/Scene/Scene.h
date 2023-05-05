#pragma once

#include "SceneCommon.h"
#include "SceneRenderer.h"
#include "Sprite.h"

namespace Omni {

	enum class OMNIFORCE_API SceneType : uint8 {
		SCENE_TYPE_2D,
		SCENE_TYPE_3D,
	};

	class OMNIFORCE_API Scene {
	public:
		Scene() = delete;
		Scene(SceneType type);

		void OnUpdate(float ts = 60.0f);
		void AddSprite(Sprite sprite) { m_DrawList2D.push_back(sprite); };
		void FlushDrawList();

	private:
		SceneRenderer* m_Renderer;
		std::vector<Sprite> m_DrawList2D;
		SceneType m_Type;
		Camera* m_Camera;

	};

}