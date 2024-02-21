#pragma once

#include "RendererCommon.h"

#include "PipelineStage.h"
#include "Image.h"

namespace Omni {

	struct PipelineBarrierInfo {
		PipelineStage src_stage;
		PipelineStage dst_stage;
		PipelineAccess src_access;
		PipelineAccess dst_access;

		Shared<Image> image;
		ImageLayout new_layout;

	};

}