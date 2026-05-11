#pragma once
#include "EditorPanel.h"

namespace Haoyue {

	struct PanelData
	{
		const char* ID = "";
		const char* Name = "";
		Ref<EditorPanel> Panel = nullptr;
		bool IsOpen = false;
	};

	class PanelManager
	{
	public:
		void OnImGuiRender();

		template<typename TPanel, typename... TArgs>
		Ref<TPanel> AddPanel()
		{

		}
	private:
		std::unordered_map<uint32_t, PanelData> m_Panels;
	};
}

