#pragma once
#include "Asset.h"

namespace Haoyue {

	struct AssetMetadata
	{
        AssetHandle Handle = 0;
		AssetType Type = AssetType::None;

		std::string FilePath;
		std::string FileName;
		std::string Extension;
		bool IsDataLoaded = false;

		bool IsValid() const { return Handle != 0; }
	};
}

