#pragma once

#include <Foundation/Common.h>
#include <RHI/RHICommon.h>

#include <RHI/PipelineStage.h>
#include <RHI/Image.h>

namespace Omni {

	struct PipelineResourceBarrierInfo {
		BitMask src_stages;
		BitMask dst_stages;
		BitMask src_access_mask;
		BitMask dst_access_mask;
		ImageLayout new_image_layout;
		uint64 buffer_barrier_size;
		uint64 buffer_barrier_offset;
	};

	struct PipelineBarrierInfo {
		std::vector<std::pair<Ref<DeviceBuffer>, PipelineResourceBarrierInfo>> buffer_barriers;
		std::vector<std::pair<Ref<Image>, PipelineResourceBarrierInfo>> image_barriers;
	};

}