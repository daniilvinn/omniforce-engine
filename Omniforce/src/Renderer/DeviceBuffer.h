#pragma once

#include "RendererCommon.h"

namespace Omni {

	/*
	*  @brief Represents a buffer used for rendering, e.g. vertex, index or storage buffers.
	*/

	enum class OMNIFORCE_API DeviceBufferFlags : BitMask
	{
		VERTEX_RATE				= BIT(0),
		INSTANCE_RATE			= BIT(1),
		INDEX_TYPE_UINT8		= BIT(2),
		INDEX_TYPE_UINT16		= BIT(3),
		INDEX_TYPE_UINT32		= BIT(4),
		CREATE_STAGING_BUFFER	= BIT(5),
	};

	enum class OMNIFORCE_API DeviceBufferUsage : uint32 {
		VERTEX_BUFFER,
		INDEX_BUFFER,
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		STAGING_BUFFER,
		BUFFER_DEVICE_ADDRESS
	};

	enum class OMNIFORCE_API DeviceBufferMemoryUsage {
		READ_BACK,
		COHERENT_WRITE,
		NO_HOST_ACCESS
	};

	enum class OMNIFORCE_API DeviceBufferMemoryHeap {
		DEVICE, // buffer is allocated in VRAM
		HOST	// buffer is allocated in system RAM
	};

	struct OMNIFORCE_API DeviceBufferSpecification
	{
		uint64 size;
		BitMask flags;
		DeviceBufferUsage buffer_usage;
		DeviceBufferMemoryUsage memory_usage;
		DeviceBufferMemoryHeap heap;
	};

	class OMNIFORCE_API DeviceBuffer 
	{
	public:
		static Shared<DeviceBuffer> Create(const DeviceBufferSpecification& spec);
		static Shared<DeviceBuffer> Create(const DeviceBufferSpecification& spec, void* data, uint64 data_size);

		virtual ~DeviceBuffer() {};

		virtual void Destroy() = 0;

		virtual DeviceBufferSpecification GetSpecification() const = 0;
		virtual uint64 GetDeviceAddress() = 0;

		virtual void UploadData(uint64 offset, void* data, uint64 data_size) = 0;
	};

}