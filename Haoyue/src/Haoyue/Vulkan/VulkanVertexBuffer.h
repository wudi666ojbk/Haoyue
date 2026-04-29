#pragma once

#include "Haoyue/Renderer/VertexBuffer.h"

#include "Haoyue/Core/Buffer.h"

#include "VulkanAllocator.h"

namespace Haoyue {

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		VulkanVertexBuffer(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);

		virtual ~VulkanVertexBuffer();

		virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		virtual void RT_SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;
		virtual const VertexBufferLayout& GetLayout() const override { return {}; }
		virtual void SetLayout(const VertexBufferLayout& layout) override {}

		virtual unsigned int GetSize() const override { return m_Size; }

		VkBuffer GetVulkanBuffer() { return m_VulkanBuffer; }
	private:
		uint32_t m_Size = 0;
		Buffer m_LocalData;

		VkBuffer m_VulkanBuffer = nullptr;
		VmaAllocation m_MemoryAllocation;
	};

}