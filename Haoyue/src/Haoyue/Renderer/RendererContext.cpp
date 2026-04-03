#include "pch.h"
#include "RendererContext.h"

#include "Haoyue/Renderer/RendererAPI.h"

#include "Haoyue/Vulkan/VulkanContext.h"

namespace Haoyue {

	Ref<RendererContext> RendererContext::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create();
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}