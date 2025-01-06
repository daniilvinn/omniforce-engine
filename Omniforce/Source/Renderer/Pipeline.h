#pragma once

#include <Foundation/Common.h>
#include <Renderer/RendererCommon.h>

#include <Renderer/DeviceBufferLayout.h>
#include <Renderer/Image.h>

#include <robin_hood.h>

namespace Omni {

#pragma region Pipeline params

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

	enum class OMNIFORCE_API PipelineDepthTestOp : uint8 {
		LESS,
		GREATER,
		GREATER_OR_EQUAL,
		EQUALS
	};
#pragma endregion

	struct OMNIFORCE_API PipelineSpecification {
		std::string debug_name;
		DeviceBufferLayout input_layout;
		Ref<Shader> shader;
		float32 line_width;
		PipelineType type;
		PipelineCullingMode culling_mode;
		PipelineFrontFace front_face;
		PipelineTopology topology;
		PipelineFillMode fill_mode;
		PipelineDepthTestOp depth_test_op;
		std::vector<ImageFormat> output_attachments_formats;
		bool primitive_restart_enable;
		bool color_blending_enable;
		bool depth_test_enable;
		bool depth_write_enable;
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
			spec.depth_test_op = PipelineDepthTestOp::GREATER_OR_EQUAL;
			spec.output_attachments_formats = {};
			spec.primitive_restart_enable = false;
			spec.color_blending_enable = true;
			spec.depth_test_enable = true;
			spec.depth_write_enable = true;
			spec.multisampling_enable = false;
			spec.sample_count = 1;

			return spec;
		}

		bool operator== (const PipelineSpecification& other) const {
			bool result = true;
			result &= shader == other.shader;
			result &= line_width == other.line_width;
			result &= type == other.type;
			result &= culling_mode == other.culling_mode;
			result &= front_face == other.front_face;
			result &= topology == other.topology;
			result &= fill_mode == other.fill_mode;
			result &= color_blending_enable == other.color_blending_enable;
			result &= depth_test_enable == other.depth_test_enable;
			result &= sample_count == other.sample_count;
			result &= input_layout.GetElements() == other.input_layout.GetElements();

			return result;
		}
	};

	class OMNIFORCE_API Pipeline {
	public:
		// NOTE: automatically(!!!) adds pipeline to pipeline library
		static Ref<Pipeline> Create(IAllocator* allocator, const PipelineSpecification& spec, UUID id = UUID());
		virtual void Destroy() = 0;

		virtual const PipelineSpecification& GetSpecification() const = 0;
		UUID GetID() const { return m_ID; }
		uint32 GetDeviceID() const {
			return uint32((m_ID >> 32) ^ m_ID);
		}
		static uint32 ComputeDeviceID(UUID id) {
			return uint32((id >> 32) ^ id);
		}

	protected:
		UUID m_ID;

	};

}

namespace robin_hood {

	template<>
	struct hash<Omni::Ref<Omni::Pipeline>> {
		size_t operator()(const Omni::Ref<Omni::Pipeline>& pipeline) const {
			return hash<Omni::uint64>()(pipeline->GetID().Get());
		}
	};

}