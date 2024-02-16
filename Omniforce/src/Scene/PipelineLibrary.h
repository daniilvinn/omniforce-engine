#pragma once 

#include "Foundation/Types.h"
#include "Renderer/Pipeline.h"
#include "Renderer/Shader.h"

namespace Omni {

	class PipelineLibrary {
	public:
		static Shared<Pipeline> GetPipeline(const PipelineSpecification& spec) {
			for (auto& pipeline : m_Pipelines) {
				if (pipeline->GetSpecification() == spec)
					return pipeline;
			}
			return nullptr;
		}

		static bool HasPipeline(const PipelineSpecification& spec) {
			for (auto& pipeline : m_Pipelines) {
				if (pipeline->GetSpecification() == spec)
					return true;
			}
			return false;
		}

		static void AddPipeline(Shared<Pipeline> pipeline) { m_Pipelines.push_back(pipeline); }

	private:
		inline static std::vector<Shared<Pipeline>> m_Pipelines;

	};

}