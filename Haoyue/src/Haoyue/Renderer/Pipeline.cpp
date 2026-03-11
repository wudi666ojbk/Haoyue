#include "pch.h"
#include "Pipeline.h"

#include "Renderer.h"

#include "Haoyue/Vulkan/VulkanPipeline.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanPipeline>::Create(spec);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}