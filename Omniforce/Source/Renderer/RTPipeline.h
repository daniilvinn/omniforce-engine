#pragma once

#include <Foundation/Common.h>
#include <Renderer/RendererCommon.h>

#include <Renderer/Shader.h>

#include <robin_hood.h>

namespace Omni {

	struct ShaderGroup {
		Ref<Shader> ray_generation;
		Ref<Shader> closest_hit;
		Ref<Shader> any_hit;
		Ref<Shader> miss;
		Ref<Shader> intersection;
	};

	struct OMNIFORCE_API RTPipelineSpecification {
		Array<ShaderGroup> groups;
		uint32 recursion_depth = 1;
	};

	class OMNIFORCE_API RTPipeline {
	public:
		// NOTE: automatically(!!!) adds pipeline to pipeline library
		static Ref<RTPipeline> Create(IAllocator* allocator, const RTPipelineSpecification& spec);
		virtual ~RTPipeline() {};

		const RTPipelineSpecification& GetSpecification() const { return m_Specification; };

	protected:
		RTPipeline(const RTPipelineSpecification& spec)
			: m_Specification(spec)
		{}

	protected:
		RTPipelineSpecification m_Specification;
	};

}