#pragma once

#include "Haoyue/Renderer/Image.h"
#include "Haoyue/Vulkan/VulkanContext.h"

#include "vulkan/vulkan.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace Haoyue {

	struct VulkanImageInfo
	{
		VkImage Image;
		VkImageView ImageView;
		VkSampler Sampler;
		VmaAllocation MemoryAlloc = nullptr;
	};

	class VulkanImage2D : public Image2D
	{
	public:
		VulkanImage2D(ImageSpecification spec);
		virtual ~VulkanImage2D();

		virtual void Invalidate() override;
		virtual void Release() override;

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual ImageSpecification& GetSpecification() override { return m_Specification; }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }

		void RT_Invalidate();

		virtual void CreatePerLayerImageViews() override;
		void RT_CreatePerLayerImageViews();
		VkImageView GetLayerImageView(uint32_t layer)
		{
			HY_CORE_ASSERT(layer < m_PerLayerImageViews.size());
			return m_PerLayerImageViews[layer];
		}

		VulkanImageInfo& GetImageInfo() { return m_Info; }
		const VulkanImageInfo& GetImageInfo() const { return m_Info; }

		const VkDescriptorImageInfo& GetDescriptor() { return m_DescriptorImageInfo; }

		virtual Buffer GetBuffer() const override { return m_ImageData; }
		virtual Buffer& GetBuffer() override { return m_ImageData; }

		virtual uint64_t GetHash() const override { return (uint64_t)m_Info.Image; }

		void UpdateDescriptor();
	private:
		ImageSpecification m_Specification;
		
		Buffer m_ImageData;

		VulkanImageInfo m_Info;
		VkDescriptorImageInfo m_DescriptorImageInfo = {};

		std::vector<VkImageView> m_PerLayerImageViews;
	};

	namespace Utils {

		inline VkFormat VulkanImageFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::RGBA:            return VK_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::RGBA32F:         return VK_FORMAT_R32G32B32A32_SFLOAT;
				case ImageFormat::DEPTH32F:        return VK_FORMAT_D32_SFLOAT;
				case ImageFormat::DEPTH24STENCIL8: return VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();
			}
			HY_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}

	}

}
