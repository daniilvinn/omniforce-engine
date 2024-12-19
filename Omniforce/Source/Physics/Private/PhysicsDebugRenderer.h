#pragma once
#if 0

#include <Foundation/Types.h>
#include <Renderer/Pipeline.h>

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>

namespace Omni {

	class PhysicsDebugRenderer : public JPH::DebugRenderer {
	public:
		static Shared<PhysicsDebugRenderer> Create();
		void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

	private:
		PhysicsDebugRenderer();
		Shared<Pipeline> m_Pipeline;

	};

}
#endif