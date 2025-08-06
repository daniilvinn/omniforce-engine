#pragma once

#include <Foundation/Common.h>
#include <RHI/RHICommon.h>

#include <RHI/RTPipeline.h>
#include <RHI/DeviceBuffer.h>

namespace Omni {

	struct SBTRegion {
		Ref<DeviceBuffer> buffer		= nullptr;
		uint32 bda_offset				= 0;
		uint64 size						= 0;
		uint64 stride					= 0;
	};

	class OMNIFORCE_API ShaderBindingTable {
	public:
		ShaderBindingTable() {}
		ShaderBindingTable(const Array<byte>& group_handles, const Array<RTShaderGroup>& groups);

		SBTRegion GetRayGenTableRegion() const { return m_RayGenTable; };
		SBTRegion GetMissTableRegion()   const { return m_MissTable; }
		SBTRegion GetHitTableRegion()    const { return m_HitTable; }

	private:
		SBTRegion m_RayGenTable;
		SBTRegion m_HitTable;
		SBTRegion m_MissTable;

		rhumap<UUID, uint64> m_ShaderHitGroupOffsets; // shader id -> group offset map
	};

}