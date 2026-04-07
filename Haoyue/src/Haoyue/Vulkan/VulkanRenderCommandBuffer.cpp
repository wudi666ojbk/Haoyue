#include "pch.h"
#include "VulkanRenderCommandBuffer.h"

#include "Haoyue/Vulkan/VulkanContext.h"

namespace Haoyue {

	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(uint32_t count)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device->GetVulkanDevice(), &cmdPoolInfo, nullptr, &m_CommandPool));

		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = m_CommandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		if (count == 0)
			count = Renderer::GetConfig().FramesInFlight;
		commandBufferAllocateInfo.commandBufferCount = count;
		m_CommandBuffers.resize(count);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &commandBufferAllocateInfo, m_CommandBuffers.data()));

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		m_WaitFences.resize(Renderer::GetConfig().FramesInFlight);
		for (auto& fence : m_WaitFences)
			VK_CHECK_RESULT(vkCreateFence(device->GetVulkanDevice(), &fenceCreateInfo, nullptr, &fence));
	}

	VulkanRenderCommandBuffer::~VulkanRenderCommandBuffer()
	{
		Renderer::SubmitResourceFree([=]()
		{
			auto device = VulkanContext::GetCurrentDevice();
			vkFreeCommandBuffers(device->GetVulkanDevice(), m_CommandPool, m_CommandBuffers.size(), m_CommandBuffers.data());
			vkDestroyCommandPool(device->GetVulkanDevice(), m_CommandPool, nullptr);
		});
	}

	void VulkanRenderCommandBuffer::Begin()
	{
		Ref<VulkanRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cmdBufInfo.pNext = nullptr;
			
			VK_CHECK_RESULT(vkBeginCommandBuffer(instance->m_CommandBuffers[Renderer::GetCurrentFrameIndex()], &cmdBufInfo));
		});
	}

	void VulkanRenderCommandBuffer::End()
	{
		Ref<VulkanRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			VK_CHECK_RESULT(vkEndCommandBuffer(instance->m_CommandBuffers[Renderer::GetCurrentFrameIndex()]));
		});
	}

	void VulkanRenderCommandBuffer::Submit()
	{
		Ref<VulkanRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			auto device = VulkanContext::GetCurrentDevice();

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			VkCommandBuffer commandBuffer = instance->m_CommandBuffers[Renderer::GetCurrentFrameIndex()];
			submitInfo.pCommandBuffers = &commandBuffer;

			VK_CHECK_RESULT(vkWaitForFences(device->GetVulkanDevice(), 1, &instance->m_WaitFences[frameIndex], VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(vkResetFences(device->GetVulkanDevice(), 1, &instance->m_WaitFences[frameIndex]));
			VK_CHECK_RESULT(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, instance->m_WaitFences[frameIndex]));
		});
	}

}