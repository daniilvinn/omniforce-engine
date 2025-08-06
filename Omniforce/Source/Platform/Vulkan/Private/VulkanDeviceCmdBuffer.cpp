#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanDeviceCmdBuffer.h>

#include <Platform/Vulkan/VulkanDebugUtils.h>
#include <Platform/Vulkan/VulkanGraphicsContext.h>
#include <Core/RuntimeExecutionContext.h>

namespace Omni {

	VulkanDeviceCmdBuffer::VulkanDeviceCmdBuffer(DeviceCmdBufferLevel level, DeviceCmdBufferType buffer_type, DeviceCmdType cmd_type)
		: m_Level(level), m_BufferType(buffer_type), m_CmdType(cmd_type)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		uint32 queue_family_index = 0;

		QueueFamilyIndex indices = device->GetPhysicalDevice()->GetQueueFamilyIndices();

		switch (cmd_type)
		{
		case DeviceCmdType::GENERAL:					queue_family_index = indices.graphics;	break;
		case DeviceCmdType::ASYNC_COMPUTE:				queue_family_index = indices.compute;	break;
		case DeviceCmdType::TRANSFER_DEDICATED:			queue_family_index = indices.transfer;	break;
		}

		VkCommandPoolCreateInfo cmd_pool_create_info = {};
		cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_create_info.queueFamilyIndex = queue_family_index;
		cmd_pool_create_info.flags = buffer_type == DeviceCmdBufferType::GENERAL ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;
		
		VK_CHECK_RESULT(vkCreateCommandPool(device->Raw(), &cmd_pool_create_info, nullptr, &m_Pool));

		VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {};
		cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_buffer_allocate_info.commandPool = m_Pool;
		cmd_buffer_allocate_info.commandBufferCount = 1;
		cmd_buffer_allocate_info.level = m_Level == DeviceCmdBufferLevel::PRIMARY ? 
			VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->Raw(), &cmd_buffer_allocate_info, &m_Buffer));
	}

	VulkanDeviceCmdBuffer::~VulkanDeviceCmdBuffer()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice()->Raw();
		auto pool = m_Pool;

		RuntimeExecutionContext::Get().GetObjectLifetimeManager().EnqueueObjectDeletion(
			[device, pool]() {
				vkDestroyCommandPool(device, pool, nullptr);
			}
		);
	}

	void VulkanDeviceCmdBuffer::Begin()
	{
		VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
		cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmd_buffer_begin_info.flags = m_BufferType == DeviceCmdBufferType::TRANSIENT ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
		VK_CHECK_RESULT(vkBeginCommandBuffer(m_Buffer, &cmd_buffer_begin_info));
	}

	void VulkanDeviceCmdBuffer::End()
	{
		VK_CHECK_RESULT(vkEndCommandBuffer(m_Buffer));
	}

	void VulkanDeviceCmdBuffer::Reset()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();
		VK_CHECK_RESULT(vkResetCommandPool(device->Raw(), m_Pool, 0));
	}

	void VulkanDeviceCmdBuffer::Execute(bool wait)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &m_Buffer;

		VkQueue queue = VK_NULL_HANDLE;
		switch (m_CmdType)
		{
		case DeviceCmdType::GENERAL:				queue = device->GetGeneralQueue();		break;
		case DeviceCmdType::ASYNC_COMPUTE:			queue = device->GetAsyncComputeQueue(); break;
		case DeviceCmdType::TRANSFER_DEDICATED:		queue = device->GetGeneralQueue();		break;
		}

		VkFence fence = VK_NULL_HANDLE;
		if (wait) {
			VkFenceCreateInfo fence_create_info = {};
			fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			vkCreateFence(device->Raw(), &fence_create_info, nullptr, &fence);
		}

		m_SubmissionMutex.lock();
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submit_info, fence));
		m_SubmissionMutex.unlock();

		if (wait) {
			vkWaitForFences(device->Raw(), 1, &fence, VK_TRUE, UINT64_MAX);
			vkDestroyFence(device->Raw(), fence, nullptr);
		}
	}

}