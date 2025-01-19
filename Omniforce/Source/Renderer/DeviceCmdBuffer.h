#pragma once

#include <Foundation/Common.h>
#include <Renderer/RendererCommon.h>

namespace Omni {

	enum class OMNIFORCE_API DeviceCmdBufferType {
		GENERAL,
		TRANSIENT
	};

	enum class OMNIFORCE_API DeviceCmdType {
		GENERAL,
		ASYNC_COMPUTE,
		TRANSFER_DEDICATED
	};

	enum class DeviceCmdBufferLevel {
		PRIMARY,
		SECONDARY
	};

	class OMNIFORCE_API DeviceCmdBuffer {
	public:
		static Ref<DeviceCmdBuffer> Create(IAllocator* allocator, DeviceCmdBufferLevel level, DeviceCmdBufferType buffer_type, DeviceCmdType cmd_type);

		virtual void Destroy() = 0;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Reset() = 0;
		virtual void Execute(bool wait) = 0;

		virtual DeviceCmdBufferLevel GetLevel() const = 0;
		virtual DeviceCmdBufferType GetBufferType() const = 0;
		virtual DeviceCmdType GetCommandType() const = 0;

	};

}