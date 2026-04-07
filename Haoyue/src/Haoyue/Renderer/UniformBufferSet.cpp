#include "pch.h"
#include "UniformBufferSet.h"

#include "Haoyue/Renderer/Renderer.h"

#include "Haoyue/Vulkan/VulkanUniformBufferSet.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {

	Ref<UniformBufferSet> UniformBufferSet::Create(uint32_t frames)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanUniformBufferSet>::Create(frames);
		}

		HY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}