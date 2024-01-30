#include "../Material.h"

#include <Memory/Pointers.hpp>

namespace Omni {

	Shared<Material> Material::Create(std::string name, AssetHandle id /*= AssetHandle()*/)
	{
		return std::make_shared<Material>(name, id);
	}

	void Material::Destroy()
	{

	}

}
