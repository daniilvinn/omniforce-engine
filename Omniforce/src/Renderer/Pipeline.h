#pragma once

#include "RendererCommon.h"
#include "DeviceBufferLayout.h"
#include "Image.h"

#include <string>

namespace Omni {

	enum class OMNIFORCE_API PipelineType : uint8 {
		GRAPHICS,
		COMPUTE,
		RAY_TRACING
	};

	enum class OMNIFORCE_API PipelineCullingMode : uint8 {
		BACK,
		FRONT,
		NONE
	};

	enum class OMNIFORCE_API PipelineFrontFace : uint8 {
		CLOCKWISE,
		COUNTER_CLOCKWISE
	};

	enum class OMNIFORCE_API PipelineTopology : uint8 {
		TRIANGLES,
		LINES,
		POINTS
	};

	enum class OMNIFORCE_API PipelineFillMode : uint8 {
		FILL,
		EDGE_ONLY
	};

	enum class PipelineStage : uint32 {

	};

	struct OMNIFORCE_API PipelineSpecification {
		std::string debug_name;
		DeviceBufferLayout input_layout;
		Shared<Shader> shader;
		float32 line_width;
		PipelineType type;
		PipelineCullingMode culling_mode;
		PipelineFrontFace front_face;
		PipelineTopology topology;
		PipelineFillMode fill_mode;
		std::vector<ImageFormat> output_attachments_formats;
		bool primitive_restart_enable;
		bool color_blending_enable;
		bool depth_test_enable;
		bool multisampling_enable;
		uint8 sample_count;

		static PipelineSpecification Default()
		{
			PipelineSpecification spec = {};
			spec.debug_name = "";
			spec.input_layout = {};
			spec.shader = nullptr;
			spec.line_width = 1.0f;
			spec.type = PipelineType::GRAPHICS;
			spec.culling_mode = PipelineCullingMode::BACK;
			spec.front_face = PipelineFrontFace::COUNTER_CLOCKWISE;
			spec.topology = PipelineTopology::TRIANGLES;
			spec.fill_mode = PipelineFillMode::FILL;
			spec.output_attachments_formats = {};
			spec.primitive_restart_enable = false;
			spec.color_blending_enable = true;
			spec.depth_test_enable = false;
			spec.multisampling_enable = false;
			spec.sample_count = 1;

			return spec;
		}
	};

	class OMNIFORCE_API Pipeline {
	public:
		static Shared<Pipeline> Create(const PipelineSpecification& spec);
		virtual void Destroy() = 0;
		virtual PipelineSpecification GetSpecification() const = 0;
	};

}