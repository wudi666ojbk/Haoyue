#pragma once

#include "pch.h"
#include "Haoyue/Core/Layer.h"

namespace Haoyue {

	class ImGuiLayer : public Layer
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;

		void SetDarkThemeColors();

		static ImGuiLayer* Create();
	};



}
