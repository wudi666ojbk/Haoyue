#pragma once

#include "Sound.h"

namespace Audio {

}

namespace Haoyue {

	class AudioComponent
	{
		UUID ParentHandle;

		AudioComponent() = default;
		explicit AudioComponent(UUID parent)
			: ParentHandle(parent)
		{
		}
	};
}

