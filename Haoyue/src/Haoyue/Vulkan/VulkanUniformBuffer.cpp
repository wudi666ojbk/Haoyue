#include "pch.h"
#include "VulkanUniformBuffer.h"

#include "VulkanContext.h"

namespace Haoyue {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		m_LocalStorage = new uint8_t[size];

		Ref<VulkanUniformBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->RT_Invalidate();
		});
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		delete[] m_LocalStorage;
	}

	void VulkanUniformBuffer::RT_Invalidate()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		// Vertex shader uniform buffer block
		VkBufferCreateInfo bufferInfo = {};
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_Size;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		// Create a new buffer
		VulkanAllocator allocator("UniformBuffer");
		m_MemoryAlloc = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_ONLY, m_Buffer);

		m_Descriptor.buffer = m_Buffer;
		m_Descriptor.offset = 0;
		m_Descriptor.range = m_Size;
	}
	
	void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(m_LocalStorage, data, size);
		Ref<VulkanUniformBuffer> instance = this;
		Renderer::Submit([instance, size, offset]() mutable
		{
			instance->RT_SetData(instance->m_LocalStorage, size, offset);
		});
	}

	void VulkanUniformBuffer::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
		VulkanAllocator allocator("VulkanUniformBuffer");
		uint8_t* pData = allocator.MapMemory<uint8_t>(m_MemoryAlloc);
		memcpy(pData, (uint8_t*)data + offset, size);
		allocator.UnmapMemory(m_MemoryAlloc);
	}

}