#pragma once

#include <nlohmann/json.hpp>
#include <Foundation/Macros.h>

namespace Omni {

	class OMNIFORCE_API Serializable {
	public:
		virtual void Serialize(nlohmann::json& node) = 0;
		virtual void Deserialize(nlohmann::json& node) = 0;

	};

}