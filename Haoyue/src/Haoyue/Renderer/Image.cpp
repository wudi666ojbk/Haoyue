#include "pch.h"
#include "Image.h"

#include "Haoyue/Vulkan/VulkanImage.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {

	Ref<Image2D> Image2D::Create(ImageSpecification spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(spec);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}