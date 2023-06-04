#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"

// WIP

namespace Omni {

	enum class AssetType {
		NONE,
		TEXTURE,
		MESH,
		SOUND_SOURCE,
		SCENE
	};

	class OMNIFORCE_API AssetHandle {
	public:
		AssetHandle();
		AssetHandle(AssetType type, uint32 data);
		~AssetHandle() {};

		AssetType GetType() const { return (AssetType)(m_Data >> 24); }
		uint32 GetData() const { return m_Data; }

	private:
		// Represents an id of asset in Asset Manager. Last byte represents AssetType.
		uint32 m_Data;
	};

}