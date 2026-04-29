#include "pch.h"
#include "VulkanFramebuffer.h"

#include "Haoyue/Renderer/Renderer.h"

#include "VulkanContext.h"

namespace Haoyue {

	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
		if (spec.Width == 0)
		{
			m_Width = Application::Get().GetWindow().GetWidth();
			m_Height = Application::Get().GetWindow().GetHeight();
		}
		else
		{
			m_Width = spec.Width;
			m_Height = spec.Height;
		}

		HY_CORE_ASSERT(spec.Attachments.Attachments.size());
		Resize(m_Width * spec.Scale, m_Height * spec.Scale, true);
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		if (!forceRecreate && (m_Width == width && m_Height == height))
			return;

		m_Width = width;
		m_Height = height;
		if (!m_Specification.SwapChainTarget)
		{
			Invalidate();
		}
		else
		{
			VulkanSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
			m_RenderPass = swapChain.GetRenderPass();
		}

		for (auto& callback : m_ResizeCallbacks)
			callback(this);
	}

	void VulkanFramebuffer::AddResizeCallback(const std::function<void(Ref<Framebuffer>)>& func)
	{
		m_ResizeCallbacks.push_back(func);
	}

	void VulkanFramebuffer::Invalidate()
	{
		Ref< VulkanFramebuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->RT_Invalidate();
		});
	}

	void VulkanFramebuffer::RT_Invalidate()
	{
		HY_CORE_TRACE("VulkanFramebuffer::RT_Invalidate");

		auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		if (m_Framebuffer)
		{
			VkFramebuffer framebuffer = m_Framebuffer;
			Renderer::SubmitResourceFree([framebuffer]()
			{
				auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
				vkDestroyFramebuffer(device, framebuffer, nullptr);
			});

			// Don't free the images if we don't own them
			if (!m_Specification.ExistingFramebuffer)
			{
				for (auto image : m_AttachmentImages)
					image->Release();

				if (m_DepthAttachmentImage)
					m_DepthAttachmentImage->Release();
			}
		}

		VulkanAllocator allocator("Framebuffer");

		std::vector<VkAttachmentDescription> attachmentDescriptions;

		std::vector<VkAttachmentReference> colorAttachmentReferences;
		VkAttachmentReference depthAttachmentReference;

		m_ClearValues.resize(m_Specification.Attachments.Attachments.size());

		bool createImages = m_AttachmentImages.empty();

		if (m_Specification.ExistingFramebuffer)
			m_AttachmentImages.clear();

		uint32_t attachmentIndex = 0;
		for (auto attachmentSpec : m_Specification.Attachments.Attachments)
		{
			if (Utils::IsDepthFormat(attachmentSpec.Format))
			{
				if (!m_Specification.ExistingFramebuffer)
				{
					if (!m_Specification.ExistingImage)
					{
						if (createImages)
						{
							ImageSpecification spec;
							spec.Format = attachmentSpec.Format;
							spec.Usage = ImageUsage::Attachment;
							spec.Width = m_Width;
							spec.Height = m_Height;
							m_DepthAttachmentImage = Image2D::Create(spec);
						}
						else
						{
							ImageSpecification& spec = m_DepthAttachmentImage->GetSpecification();
							spec.Width = m_Width;
							spec.Height = m_Height;
						}

						Ref<VulkanImage2D> depthAttachmentImage = m_DepthAttachmentImage.As<VulkanImage2D>();
						depthAttachmentImage->RT_Invalidate(); // Create immediately
					}
					else
					{
						m_DepthAttachmentImage = m_Specification.ExistingImage;
					}
				}
				else
				{
					Ref<VulkanFramebuffer> existingFramebuffer = m_Specification.ExistingFramebuffer.As<VulkanFramebuffer>();
					m_DepthAttachmentImage = existingFramebuffer->GetDepthImage();
				}

				VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
				attachmentDescription.flags = 0;
				attachmentDescription.format = Utils::VulkanImageFormat(attachmentSpec.Format);
				attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescription.loadOp = m_Specification.ClearOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
				attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
				attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachmentDescription.initialLayout = m_Specification.ClearOnLoad ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				if (attachmentSpec.Format == ImageFormat::DEPTH24STENCIL8 || true) // Separate layouts requires a "separate layouts" flag to be enabled
				{
					attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // TODO: if not sampling
					attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // TODO: if sampling
					depthAttachmentReference = { attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
				}
				else
				{
					attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL; // TODO: if not sampling
					attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL; // TODO: if sampling
					depthAttachmentReference = { attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL };
				}
				m_ClearValues[attachmentIndex].depthStencil = { 1.0f, 0 };
			}
			else
			{
				HY_CORE_ASSERT(!m_Specification.ExistingImage, "Not supported for color attachments");

				Ref<VulkanImage2D> colorAttachment;
				if (!m_Specification.ExistingFramebuffer)
				{
					if (createImages)
					{
						ImageSpecification spec;
						spec.Format = attachmentSpec.Format;
						spec.Usage = ImageUsage::Attachment;
						spec.Width = m_Width;
						spec.Height = m_Height;
						colorAttachment = m_AttachmentImages.emplace_back(Image2D::Create(spec)).As<VulkanImage2D>();
					}
					else
					{
						Ref<Image2D> image = m_AttachmentImages[attachmentIndex];
						ImageSpecification& spec = image->GetSpecification();
						spec.Width = m_Width;
						spec.Height = m_Height;
						colorAttachment = image.As<VulkanImage2D>();
					}

					colorAttachment->RT_Invalidate(); // Create immediately
				}
				else
				{
					Ref<VulkanFramebuffer> existingFramebuffer = m_Specification.ExistingFramebuffer.As<VulkanFramebuffer>();
					Ref<Image2D> existingImage = existingFramebuffer->GetImage(attachmentIndex);
					colorAttachment = m_AttachmentImages.emplace_back(existingImage).As<VulkanImage2D>();
				}


				VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
				attachmentDescription.flags = 0;
				attachmentDescription.format = Utils::VulkanImageFormat(attachmentSpec.Format);
				attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescription.loadOp = m_Specification.ClearOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
				attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
				attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachmentDescription.initialLayout = m_Specification.ClearOnLoad ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				const auto& clearColor = m_Specification.ClearColor;
				m_ClearValues[attachmentIndex].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
				colorAttachmentReferences.emplace_back(VkAttachmentReference{ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			}

			attachmentIndex++;
		}

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = colorAttachmentReferences.size();
		subpassDescription.pColorAttachments = colorAttachmentReferences.data();
		if (m_DepthAttachmentImage)
			subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

		// TODO: do we need these?
		// Use subpass dependencies for layout transitions
		std::vector<VkSubpassDependency> dependencies;

		if (m_AttachmentImages.size())
		{
			{
				VkSubpassDependency& depedency = dependencies.emplace_back();
				depedency.srcSubpass = VK_SUBPASS_EXTERNAL;
				depedency.dstSubpass = 0;
				depedency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				depedency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				depedency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				depedency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				depedency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}
			{
				VkSubpassDependency& depedency = dependencies.emplace_back();
				depedency.srcSubpass = 0;
				depedency.dstSubpass = VK_SUBPASS_EXTERNAL;
				depedency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				depedency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				depedency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				depedency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				depedency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}
		}

		if (m_DepthAttachmentImage)
		{
			{
				VkSubpassDependency& depedency = dependencies.emplace_back();
				depedency.srcSubpass = VK_SUBPASS_EXTERNAL;
				depedency.dstSubpass = 0;
				depedency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				depedency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				depedency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				depedency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				depedency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}

			{
				VkSubpassDependency& depedency = dependencies.emplace_back();
				depedency.srcSubpass = 0;
				depedency.dstSubpass = VK_SUBPASS_EXTERNAL;
				depedency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				depedency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				depedency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				depedency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				depedency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}
		}

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass));

		std::vector<VkImageView> attachments(m_AttachmentImages.size());
		for (uint32_t i = 0; i < m_AttachmentImages.size(); i++)
		{
			Ref<VulkanImage2D> image = m_AttachmentImages[i].As<VulkanImage2D>();
			attachments[i] = image->GetImageInfo().ImageView;
		}

		if (m_DepthAttachmentImage)
		{
			Ref<VulkanImage2D> image = m_DepthAttachmentImage.As<VulkanImage2D>();
			if (m_Specification.ExistingImage)
			{
				attachments.emplace_back(image->GetLayerImageView(m_Specification.ExistingImageLayer));
			}
			else
			{
				attachments.emplace_back(image->GetImageInfo().ImageView);
			}
		}

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_RenderPass;
		framebufferCreateInfo.attachmentCount = attachments.size();
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.width = m_Width;
		framebufferCreateInfo.height = m_Height;
		framebufferCreateInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &m_Framebuffer));
	}

}