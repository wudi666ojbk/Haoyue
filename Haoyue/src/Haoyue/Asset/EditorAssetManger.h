#pragma once

#include "Haoyue/Asset/AssetTypes.h"
#include "Haoyue/Asset/Asset.h"

namespace Haoyue {
	class EditorAssetManger
	{
	public:
		static AssetType GetAssetTypeFromExtension(const std::string& fileName);
		static AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
	private:
	};
}
