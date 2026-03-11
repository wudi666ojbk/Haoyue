#include "pch.h"
#include "Texture.h"

#include "Haoyue/Renderer/RendererAPI.h"
#include "Haoyue/Vulkan/VulkanTexture.h"


namespace Haoyue {

	Ref<Texture2D> Texture2D::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(format, width, height, data, properties);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path, TextureProperties properties)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(path, properties);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTextureCube>::Create(format, width, height, data, properties);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(const std::string& path, TextureProperties properties)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanTextureCube>::Create(path, properties);
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	

	

}