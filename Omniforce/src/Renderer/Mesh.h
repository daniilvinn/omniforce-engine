#pragma once

#include "RendererCommon.h"

namespace Omni {

	class OMNIFORCE_API Mesh {
	public:
		Mesh(Shared<DeviceBuffer> vertex_buffer, Shared<DeviceBuffer> index_buffer);
		

	private:
		Shared<DeviceBuffer> m_VertexBuffer;
		Shared<DeviceBuffer> m_IndexBuffer;

	};

}