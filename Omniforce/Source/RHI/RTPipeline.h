#pragma once

#include <Foundation/Common.h>
#include <RHI/RHICommon.h>

#include <RHI/Shader.h>

#include <robin_hood.h>

namespace Omni {

	struct RTShaderGroup {
		Ref<Shader> ray_generation = nullptr;
		Ref<Shader> closest_hit = nullptr;
		Ref<Shader> any_hit = nullptr;
		Ref<Shader> miss = nullptr;
		Ref<Shader> intersection = nullptr;
	};

	struct OMNIFORCE_API RTPipelineSpecification {
		Array<RTShaderGroup> groups;
		uint32 recursion_depth = 1;
	};

	class OMNIFORCE_API RTPipeline {
	public:
		// NOTE: automatically(!!!) adds pipeline to pipeline library
		static Ref<RTPipeline> Create(IAllocator* allocator, const RTPipelineSpecification& spec);
		virtual ~RTPipeline() {};

		const RTPipelineSpecification& GetSpecification() const { return m_Specification; };
		virtual const ShaderBindingTable& GetSBT() const = 0;

	protected:
		RTPipeline(const RTPipelineSpecification& spec)
		{
			m_Specification = spec;
		}

	protected:
		RTPipelineSpecification m_Specification = {};
	};

}