#include "pch.h"
#include "EditorAssetManger.h"

#include <filesystem>

namespace Haoyue {
	AssetType EditorAssetManger::GetAssetTypeFromExtension(const std::string& fileName)
	{
		size_t dotPos = fileName.find_last_of('.');
		if (dotPos == std::string::npos)
			return AssetType::None;

		std::string extension = fileName.substr(dotPos);
		auto it = s_AssetExtensionMap.find(extension);
		if (it != s_AssetExtensionMap.end())
			return it->second;

		return AssetType::None;
	}

	AssetType EditorAssetManger::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}
}