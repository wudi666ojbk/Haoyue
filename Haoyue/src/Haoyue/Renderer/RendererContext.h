#pragma once

#include "Haoyue/Core/Ref.h"

struct GLFWwindow;

namespace Haoyue {

	class RendererContext : public RefCounted
	{
	public:
		RendererContext() = default;
		virtual ~RendererContext() = default;

		virtual void Init() = 0;

		static Ref<RendererContext> Create();
	};

}
