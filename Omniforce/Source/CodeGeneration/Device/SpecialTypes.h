#pragma once

#include <Foundation/Common.h>

#include <Renderer/DeviceBuffer.h>

namespace Omni {

	// This struct is templated because template param is needed by code generation
	template<typename T>
	struct BDA {
		BDA() {}

		BDA(WeakPtr<DeviceBuffer> buffer, uint64 offset = 0)
			: address(buffer->GetDeviceAddress() + offset)
		{}

		uint64 address = 0;
	};

	template<typename T>
	struct ShaderRuntimeArray {
		const uint32 _;
	};

}