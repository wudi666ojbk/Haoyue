#pragma once

#include "Haoyue/Renderer/IndexBuffer.h"

#include "Haoyue/Core/Buffer.h"

#include "VulkanAllocator.h"

namespace Haoyue {

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(uint32_t size);
		VulkanIndexBuffer(void* data, uint32_t size = 0);
		virtual ~VulkanIndexBuffer();

		virtual uint32_t GetCount() const override { return m_Size / sizeof(uint32_t); }

		virtual uint32_t GetSize() const override { return m_Size; }

		VkBuffer GetVulkanBuffer() { return m_VulkanBuffer; }
	private:
		uint32_t m_Size = 0;
		Buffer m_LocalData;

		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation;
	};

}