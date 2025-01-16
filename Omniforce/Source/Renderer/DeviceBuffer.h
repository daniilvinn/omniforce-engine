#pragma once

#include <Foundation/Common.h>
#include <Renderer/RendererCommon.h>

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
		SHADER_DEVICE_ADDRESS,
		INDIRECT_PARAMS
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
		OMNI_DEBUG_ONLY_FIELD(std::string debug_name);
	};

	class OMNIFORCE_API DeviceBuffer 
	{
	public:
		static Ref<DeviceBuffer> Create(IAllocator* allocator, const DeviceBufferSpecification& spec);
		static Ref<DeviceBuffer> Create(IAllocator* allocator, const DeviceBufferSpecification& spec, void* data, uint64 data_size);

		virtual ~DeviceBuffer() {};

		virtual DeviceBufferSpecification GetSpecification() const = 0;
		virtual uint64 GetDeviceAddress() = 0;
		virtual uint64 GetPerFrameSize() = 0;
		virtual uint64 GetFrameOffset() = 0;

		virtual void CopyRegionTo(Ref<DeviceCmdBuffer> cmd_buffer, Ref<DeviceBuffer> dst_buffer, uint64 src_offset, uint64 dst_offset, uint64 size) = 0;
		virtual void UploadData(uint64 offset, void* data, uint64 data_size) = 0;
		virtual void Clear(Ref<DeviceCmdBuffer> cmd_buffer, uint64 offset, uint64 size, uint32 value) = 0;
	};

}