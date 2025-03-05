#pragma once

#include <Foundation/Common.h>

#include <Renderer/DeviceBuffer.h>

namespace Omni {

	// This struct is templated because template param is needed by code generation
	template<typename T>
	struct BDA {
		BDA() {}

		BDA(WeakPtr<DeviceBuffer> buffer, bool split_by_fif = false)
		{
			address = buffer->GetDeviceAddress();
			address += split_by_fif ? buffer->GetFrameOffset() : 0;
		}

		BDA(WeakPtr<DeviceBuffer> buffer, uint64 offset)
			: address(buffer->GetDeviceAddress() + offset)
		{}

		uint64 address;
	};

}