#pragma once

#include "RHICommon.h"

#include <filesystem>
#include <vector>
#include <map>

namespace Omni {
	
	enum class OMNIFORCE_API ShaderStage : uint32 {
		VERTEX,
		TASK,
		MESH,
		FRAGMENT,
		COMPUTE,
		RAYGEN,
		CLOSESTHIT,
		ANYHIT,
		MISS,
		INTERSECTION,
		CALLABLE,
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
		static Ref<Shader> Create(IAllocator* allocator, std::map<ShaderStage, std::vector<uint32>> binaries, std::filesystem::path path);
		virtual ~Shader() {};
		virtual void Destroy() {};

		UUID GetID() const { return m_ID; }

		bool Dirty() const { return m_Dirty; };
		void SetDirty(bool dirty) { m_Dirty = dirty; };
		void Unload() { Destroy(); m_Dirty = true; };

		virtual void RestoreShaderModule(std::filesystem::path path) = 0;

		bool operator==(Ref<Shader> other) { return m_ID == other->m_ID; }

	protected:
		UUID m_ID;
		bool m_Dirty;
	};

	namespace Utils {
		inline constexpr std::string ShaderStageToString(const ShaderStage& stage) {
			switch (stage)
			{
			case ShaderStage::VERTEX:		return "vertex";
			case ShaderStage::FRAGMENT:		return "fragment";
			case ShaderStage::COMPUTE:		return "compute";
			case ShaderStage::TASK:			return "task";
			case ShaderStage::MESH:			return "mesh";
			case ShaderStage::UNKNOWN:		return "unknown";
			default:						std::unreachable();
			}
			return "unknown";
		}

	}

}