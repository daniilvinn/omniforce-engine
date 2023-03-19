#pragma once

#include "RendererCommon.h"

#include <string>

namespace Omni {

	enum class OMNIFORCE_API PipelineType : uint32 {
		GRAPHICS,
		COMPUTE,
		RAY_TRACING
	};

	enum class OMNIFORCE_API PipelineCullingMode {
		BACK,
		FRONT,
		NONE
	};

	enum class PipelineFrontFace {
		CLOCK_WISE,
		COUNTER_CLOCK_WISE,
		BOTH
	};

	struct OMNIFORCE_API PipelineSpecification {
		PipelineType type;
		PipelineCullingMode culling_mode;
		PipelineFrontFace front_face;
		std::string debug_name;
	};

	class OMNIFORCE_API Pipeline {

		static Shared<Pipeline> Create(const PipelineSpecification& spec);

	};

}