#pragma once

#include "RendererCommon.h"

#include <filesystem>
#include <vector>
#include <map>

namespace Omni {
	
	enum class OMNIFORCE_API ShaderStage : uint32 {
		VERTEX,
		FRAGMENT,
		COMPUTE,
		UNKNOWN
	};

	enum class OMNIFORCE_API DeviceDataType : uint32 {
		INT, INT2, INT3, INT4,
		FLOAT, FLOAT2, FLOAT3, FLOAT4,
		IMAT3, IMAT4, MAT3, MAT4
	};

	constexpr uint32 DeviceDataTypeSize(const DeviceDataType& type) {
		switch (type)
		{
		case DeviceDataType::INT:			return 4;
		case DeviceDataType::INT2:			return 4 * 2;
		case DeviceDataType::INT3:			return 4 * 3;
		case DeviceDataType::INT4:			return 4 * 4;
		case DeviceDataType::FLOAT:			return 4;
		case DeviceDataType::FLOAT2:		return 4 * 2;
		case DeviceDataType::FLOAT3:		return 4 * 3;
		case DeviceDataType::FLOAT4:		return 4 * 4;
		case DeviceDataType::IMAT3:			return 4 * 3 * 3;
		case DeviceDataType::IMAT4:			return 4 * 4 * 4;
		case DeviceDataType::MAT3:			return 4 * 3 * 3;
		case DeviceDataType::MAT4:			return 4 * 4 * 4;
		default:							std::unreachable();
		}
	}

	class OMNIFORCE_API Shader {
	public:
		static Shared<Shader> Create(std::map<ShaderStage, std::vector<uint32>> binaries, std::filesystem::path path);
		virtual ~Shader() {};
		virtual void Destroy() = 0;

		virtual bool Dirty() const = 0;
		virtual void SetDirty(bool dirty) = 0;

		virtual void RestoreShaderModule(std::filesystem::path path) = 0;
	};

}