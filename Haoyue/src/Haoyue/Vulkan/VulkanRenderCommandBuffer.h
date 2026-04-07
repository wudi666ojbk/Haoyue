#pragma once

#include "Haoyue/Renderer/RenderCommandBuffer.h"
#include "vulkan/vulkan.h"

namespace Haoyue {

	class VulkanRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		VulkanRenderCommandBuffer(uint32_t count = 0);
		~VulkanRenderCommandBuffer();

		virtual void Begin() override;
		virtual void End() override;
		virtual void Submit() override;

		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
		{
			HY_CORE_ASSERT(frameIndex < m_CommandBuffers.size());
			return m_CommandBuffers[frameIndex];
		}
	private:
		VkCommandPool m_CommandPool = nullptr;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		std::vector<VkFence> m_WaitFences;

		int m_ActiveBufferIndex = -1;
	};

}
