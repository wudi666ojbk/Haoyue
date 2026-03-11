#include "pch.h"
#include "Material.h"

#include "Haoyue/Vulkan/VulkanMaterial.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {

	Ref<Material> Material::Create(const Ref<Shader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanMaterial>::Create(shader, name);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	
}