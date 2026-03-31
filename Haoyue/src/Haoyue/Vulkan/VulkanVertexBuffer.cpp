#include "pch.h"
#include "VulkanVertexBuffer.h"

#include "VulkanContext.h"

#include "Haoyue/Renderer/Renderer.h"

namespace Haoyue {

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size, VertexBufferUsage usage)
		: m_Size(size)
	{
	}

	VulkanVertexBuffer::VulkanVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage)
		: m_Size(size)
	{
		m_LocalData = Buffer::Copy(data, size);

		Ref<VulkanVertexBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			// TODO: Use staging
			auto device = VulkanContext::GetCurrentDevice();
			VulkanAllocator allocator("VertexBuffer");

			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = instance->m_Size;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

			// Copy data to staging buffer
			uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
			memcpy(destData, instance->m_LocalData.Data, instance->m_LocalData.Size);
			allocator.UnmapMemory(stagingBufferAllocation);

			VkBufferCreateInfo vertexBufferCreateInfo = {};
			vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexBufferCreateInfo.size = instance->m_Size;
			vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			VmaAllocation bufferAlloc = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, instance->m_VulkanBuffer);

			VkCommandBuffer copyCmd = device->GetCommandBuffer(true);
			VkBufferCopy copyRegion = {};

			copyRegion.size = instance->m_LocalData.Size;
			vkCmdCopyBuffer(
				copyCmd,
				stagingBuffer,
				instance->m_VulkanBuffer,
				1,
				&copyRegion);

			device->FlushCommandBuffer(copyCmd);
		});
	}


}