#pragma once

namespace Haoyue {

	class EditorPanel : public RefCounted
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnImGuiRender() = 0;
	};
}

