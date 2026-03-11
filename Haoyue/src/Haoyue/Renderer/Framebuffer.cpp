#include "pch.h"
#include "Framebuffer.h"

#include "Haoyue/Vulkan/VulkanFramebuffer.h"

#include "Haoyue/Renderer/RendererAPI.h"

namespace Haoyue {

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		Ref<Framebuffer> result = nullptr;

		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:		return nullptr;
			case RendererAPIType::Vulkan:	result = Ref<VulkanFramebuffer>::Create(spec); break;
		}
		FramebufferPool::GetGlobal()->Add(result);
		return result;
	}

	FramebufferPool* FramebufferPool::s_Instance = new FramebufferPool;

	FramebufferPool::FramebufferPool(uint32_t maxFBs /* = 32 */)
	{
	}

	FramebufferPool::~FramebufferPool()
	{
		
	}

	std::weak_ptr<Framebuffer> FramebufferPool::AllocateBuffer()
	{
		// m_Pool.push_back();
		return std::weak_ptr<Framebuffer>();
	}

	void FramebufferPool::Add(const Ref<Framebuffer>& framebuffer)
	{
		m_Pool.push_back(framebuffer);
	}

}