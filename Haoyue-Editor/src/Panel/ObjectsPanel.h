#pragma once

#include "Haoyue/Asset/AssetManager.h"

namespace Haoyue {

	class ObjectsPanel
	{
	public:
		ObjectsPanel();

		void OnImGuiRender();

	private:
		void DrawObject(const char* label, AssetHandle handle);
	};

}
