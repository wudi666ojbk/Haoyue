#include "pch.h"
#include "UniformBuffer.h"

#include "Haoyue/Renderer/Renderer.h"

#include "Haoyue/Vulkan/VulkanUniformBuffer.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {
    Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
    {
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanUniformBuffer>::Create(size, binding);
		}

		HY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
    }
}