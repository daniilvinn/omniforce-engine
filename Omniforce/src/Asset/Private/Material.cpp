#include "../Material.h"

#include <Log/Logger.h>
#include <Foundation/Macros.h>
#include <Memory/Pointers.hpp>

namespace Omni {

	Shared<Material> Material::Create(std::string name, AssetHandle id /*= AssetHandle()*/)
	{
		return std::make_shared<Material>(name, id);
	}

	uint8 Material::GetRuntimeEntrySize(uint8 variant_index)
	{
		switch (variant_index)
		{
		case 0: return 4;
		case 1: return 4;
		case 2: return 4;
		case 3: return 16;
		default: OMNIFORCE_ASSERT_TAGGED(false, "Invalid material entry type index");
		}

		return 0;
	}

	void Material::Destroy()
	{
		
	}

}
