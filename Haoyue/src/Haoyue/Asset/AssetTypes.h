#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <cstring>

namespace Haoyue {

	enum class AssetType : int8_t
	{
		Scene,
		Mesh,
		Texture,
		EnvMap,
		Audio,
		Script,
		PhysicsMat,
		Directory,
		Other,
		None,
		Missing
	};

	inline static std::unordered_map<std::string, AssetType> s_AssetExtensionMap =
	{
		// 引擎内置资源
		{ ".hsc", AssetType::Scene },
		{ ".cs", AssetType::Script },

		// Textures
		{ ".png", AssetType::Texture },
		{ ".jpg", AssetType::Texture },
		{ ".jpeg", AssetType::Texture },
		{ ".hdr", AssetType::EnvMap },

		{ ".wav", AssetType::Audio },
		{ ".ogg", AssetType::Audio },

		// Modle
		{ ".fbx", AssetType::Mesh},
		{ ".obj", AssetType::Mesh},
		{ ".hpm", AssetType::PhysicsMat},
	};

	namespace Utils {

		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")        return AssetType::None;
			if (assetType == "Scene")       return AssetType::Scene;
			if (assetType == "Mesh")        return AssetType::Mesh;
			if (assetType == "Texture")     return AssetType::Texture;
			if (assetType == "EnvMap")      return AssetType::EnvMap;
			if (assetType == "Audio")       return AssetType::Audio;
			if (assetType == "Script")      return AssetType::Script;
			if (assetType == "PhysicsMat")  return AssetType::PhysicsMat;
			if (assetType == "Directory")   return AssetType::Directory;
			if (assetType == "Other")       return AssetType::Other;
			if (assetType == "Missing")     return AssetType::Missing;

			return AssetType::None;
		}

		inline const char* AssetTypeToString(AssetType assetType)
		{
			switch (assetType)
			{
			case AssetType::None:        return "None";
			case AssetType::Scene:       return "Scene";
			case AssetType::Mesh:        return "Mesh";
			case AssetType::Texture:     return "Texture";
			case AssetType::EnvMap:      return "EnvMap";
			case AssetType::Audio:       return "Audio";
			case AssetType::Script:      return "Script";
			case AssetType::PhysicsMat:  return "PhysicsMat";
			case AssetType::Directory:   return "Directory";
			case AssetType::Other:       return "Other";
			case AssetType::Missing:     return "Missing";
			}

			return "None";
		}

	}
}
