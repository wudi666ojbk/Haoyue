#pragma once
#include "Haoyue/Renderer/UniformBuffer.h"

#include "VulkanAllocator.h"

namespace Haoyue {

	class VulkanUniformBuffer :	public UniformBuffer
	{
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		virtual ~VulkanUniformBuffer();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual uint32_t GetBinding() const override { return m_Binding; }

		const VkDescriptorBufferInfo& GetDescriptorBufferInfo() const { return m_Descriptor; }
	private:
		void RT_Invalidate();
		void RT_SetData(const void* data, uint32_t size, uint32_t offset);
	private:
		VmaAllocation m_MemoryAlloc = nullptr;
		VkBuffer m_Buffer;
		VkDescriptorBufferInfo m_Descriptor;
		uint32_t m_Size = 0;
		uint32_t m_Binding = 0;
		std::string m_Name;
		VkShaderStageFlagBits m_ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

		uint8_t* m_LocalStorage = nullptr;
	};
}

