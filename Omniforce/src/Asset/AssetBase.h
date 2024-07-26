#pragma once

#include "Foundation/Types.h"
#include "AssetType.h"

namespace Omni {

	class AssetBase {
	public:
		AssetHandle Handle;
		AssetType Type;

		virtual void Destroy() = 0;
		bool Valid() { return Handle.Valid(); }

	};

}