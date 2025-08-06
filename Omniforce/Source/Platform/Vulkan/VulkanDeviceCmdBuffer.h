#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <RHI/DeviceCmdBuffer.h>

#include <shared_mutex>

namespace Omni {

	class VulkanDeviceCmdBuffer : public DeviceCmdBuffer {
	public:
		VulkanDeviceCmdBuffer(DeviceCmdBufferLevel level, DeviceCmdBufferType buffer_type, DeviceCmdType cmd_type);
		~VulkanDeviceCmdBuffer();

		void Begin() override;
		void End() override;
		void Reset() override;
		void Execute(bool wait) override;

		VkCommandBuffer Raw() const { return m_Buffer; }
		VkCommandPool RawPool() const { return m_Pool; }

		DeviceCmdBufferLevel GetLevel() const override { return m_Level; }
		DeviceCmdBufferType GetBufferType() const override { return m_BufferType; }
		DeviceCmdType GetCommandType() const override { return m_CmdType; }

		operator VkCommandBuffer() { return m_Buffer; }

	private:
		VkCommandPool m_Pool;
		VkCommandBuffer m_Buffer;

		DeviceCmdBufferLevel m_Level;
		DeviceCmdBufferType m_BufferType;
		DeviceCmdType m_CmdType;

		inline static std::shared_mutex m_SubmissionMutex;
	};

}