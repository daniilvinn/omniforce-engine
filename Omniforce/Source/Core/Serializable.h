#pragma once

#include <Foundation/Common.h>

#include <nlohmann/json.hpp>

namespace Omni {

	class OMNIFORCE_API Serializable {
	public:
		virtual void Serialize(nlohmann::json& node) = 0;
		virtual void Deserialize(nlohmann::json& node) = 0;

	};

}