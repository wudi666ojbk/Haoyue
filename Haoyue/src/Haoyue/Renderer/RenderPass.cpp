#include "pch.h"
#include "RenderPass.h"

#include "Renderer.h"

#include "Haoyue/Vulkan/VulkanRenderPass.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    HY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderPass>::Create(spec);
		}

		HY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}