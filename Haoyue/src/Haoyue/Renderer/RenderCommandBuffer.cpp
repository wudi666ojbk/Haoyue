#include "pch.h"
#include "RenderCommandBuffer.h"

#include "Haoyue/Vulkan/VulkanRenderCommandBuffer.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create(uint32_t count, const std::string& debugName)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(count);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}