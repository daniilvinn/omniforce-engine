#pragma once 

#include <Foundation/Common.h>

#include <Renderer/Pipeline.h>
#include <Renderer/Shader.h>

#include <shared_mutex>

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

		static void AddPipeline(Shared<Pipeline> pipeline) { 
			std::lock_guard lock(m_Mutex);
			m_Pipelines.push_back(pipeline); 
		}

	private:
		inline static std::vector<Shared<Pipeline>> m_Pipelines;
		inline static std::shared_mutex m_Mutex;

	};

}