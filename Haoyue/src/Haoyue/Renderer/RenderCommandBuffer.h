#pragma once

#include "Haoyue/Core/Ref.h"

namespace Haoyue {

	class RenderCommandBuffer : public RefCounted
	{
	public:
		virtual ~RenderCommandBuffer() {}

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Submit() = 0;

		static Ref<RenderCommandBuffer> Create(uint32_t count = 0, const std::string& debugName = "");
	};

}
