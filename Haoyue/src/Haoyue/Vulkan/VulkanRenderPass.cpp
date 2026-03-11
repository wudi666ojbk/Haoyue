#include "pch.h"
#include "VulkanRenderPass.h"

namespace Haoyue {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
	}

}