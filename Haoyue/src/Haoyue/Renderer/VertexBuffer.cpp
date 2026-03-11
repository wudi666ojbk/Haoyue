#include "pch.h"
#include "VertexBuffer.h"

#include "Renderer.h"

#include "Haoyue/Vulkan/VulkanVertexBuffer.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {

	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(data, size, usage);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(size, usage);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}