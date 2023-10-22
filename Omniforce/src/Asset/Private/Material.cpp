#include "../Material.h"

namespace Omni {

	Material::Material(std::string name, AssetHandle id /*= UUID()*/)
	{
		m_Name = std::move(name);
		Type = AssetType::MATERIAL;
		Handle = id;
	}

	void Material::Destroy()
	{

	}

}
