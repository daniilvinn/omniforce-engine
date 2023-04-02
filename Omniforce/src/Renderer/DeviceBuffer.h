#pragma once

#include "RendererCommon.h"

namespace Omni {

	/*
	*  @brief Represents a buffer used for rendering, e.g. vertex, index or storage buffers.
	*/

	enum class OMNIFORCE_API DeviceBufferFlags : BitMask
	{
		VERTEX_RATE = BIT(0),
		INSTANCE_RATE = BIT(1),
		INDEX_TYPE_UINT8 = BIT(2),
		INDEX_TYPE_UINT16 = BIT(3),
		INDEX_TYPE_UINT32 = BIT(4),
		CREATE_STAGING_BUFFER = BIT(5)
	};

	enum class OMNIFORCE_API DeviceBufferUsage : uint32 {
		VERTEX_BUFFER,
		INDEX_BUFFER,
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		STAGING_BUFFER
	};

	enum class OMNIFORCE_API DeviceBufferMemoryUsage {
		FREQUENT_ACCESS,
		ONE_TIME_HOST_ACCESS,
		NO_HOST_ACCESS
	};

	struct OMNIFORCE_API DeviceBufferSpecification
	{
		uint64 size;
		BitMask flags;
		DeviceBufferUsage buffer_usage;
		DeviceBufferMemoryUsage memory_usage;
	};

	class OMNIFORCE_API DeviceBuffer 
	{
	public:
		static Shared<DeviceBuffer> Create(const DeviceBufferSpecification& spec);
		static Shared<DeviceBuffer> Create(const DeviceBufferSpecification& spec, void* data, uint64 data_size);

		virtual ~DeviceBuffer() {};

		virtual void Destroy() = 0;

		virtual DeviceBufferSpecification GetSpecification() const = 0;
	};

}