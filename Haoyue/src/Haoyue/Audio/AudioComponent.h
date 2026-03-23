#pragma once

#include "Sound.h"

namespace Audio {

}

namespace Haoyue {

	struct AudioComponent
	{
		UUID ParentHandle;

		AudioComponent() = default;
		explicit AudioComponent(UUID parent)
			: ParentHandle(parent)
		{
		}
	};
}

