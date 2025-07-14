#pragma once

#include <Foundation/Common.h>
#include <Renderer/RendererCommon.h>

#include <Asset/Material.h>

namespace Omni {

	class RTAccelerationStructure;

	struct BLASBuildInfo {
		Ref<DeviceBuffer> geometry		= nullptr;
		Ref<DeviceBuffer> indices		= nullptr;
		uint32 vertex_stride			= (uint32)-1;
		uint32 vertex_count				= (uint32)-1;
		uint32 index_count				= (uint32)-1;
		MaterialDomain domain	= MaterialDomain::NONE;
	};

	struct TLASInstance {
		WeakPtr<RTAccelerationStructure> blas;
		Transform transform							= {};
		uint32 custom_index							= (uint32)-1;
		uint32 mask									= (uint32)-1;
		uint32 SBT_record_offset					= (uint32)-1;
	};

	struct TLASBuildInfo {
		Array<TLASInstance> instances;
	};

	class RTAccelerationStructure {
	public:
		static Ptr<RTAccelerationStructure> Create(IAllocator* allocator, const BLASBuildInfo& build_info);
		static Ptr<RTAccelerationStructure> Create(IAllocator* allocator, const TLASBuildInfo& build_info);

		virtual ~RTAccelerationStructure() {};

	protected:
		RTAccelerationStructure()
		{}
	};

}