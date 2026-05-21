#pragma once

#include "Haoyue/Renderer/SceneRenderer.h"

namespace Haoyue {

	class SceneRendererPanel
	{
	public:
		SceneRendererPanel(Ref<SceneRenderer> renderer);

		void OnImGuiRender();
	private:
		Ref<SceneRenderer> m_Renderer;
	};

}
