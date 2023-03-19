#pragma once

#include "RendererCommon.h"

namespace Cursed {

	/*
	*  @brief Represents a buffer used for rendering, e.g. vertex, index or storage buffers.
	*/

	enum class DeviceBufferFlags : BitMask
	{
		VERTEX_RATE = BIT(0),
		INSTANCE_RATE = BIT(1),
		INDEX_TYPE_UINT8 = BIT(2),
		INDEX_TYPE_UINT16 = BIT(3),
		INDEX_TYPE_UINT32 = BIT(4),
		CREATE_STAGING_BUFFER = BIT(5)
	};

	enum class DeviceBufferUsage : uint32 {
		VERTEX_BUFFER,
		INDEX_BUFFER,
		UNIFORM_BUFFER,
		STORAGE_BUFFER
	};

	enum class DeviceBufferMemoryUsage {
		FREQUENT_ACCESS,
		ONE_TIME_HOST_ACCESS,
		NO_HOST_ACCESS
	};

	struct DeviceBufferSpecification 
	{
		uint64 size;
		BitMask flags;
		DeviceBufferUsage buffer_usage;
		DeviceBufferMemoryUsage memory_usage;
	};

	class CURSED_API DeviceBuffer 
	{
	public:
		static Shared<DeviceBuffer> Create(const DeviceBufferSpecification& spec);
		static Shared<DeviceBuffer> Create(const DeviceBufferSpecification& spec, void* data, uint64 data_size);

		virtual ~DeviceBuffer() {};

		virtual void Destroy() = 0;

		virtual DeviceBufferSpecification GetSpecification() const = 0;
	};

}