#include "pch.h"
#include "AssetImporter.h"

namespace Haoyue {

	void AssetImporter::Init()
	{
		s_Serializers[AssetType::Texture] = CreateScope<TextureSerializer>();
		s_Serializers[AssetType::Mesh] = CreateScope<MeshAssetSerializer>();
		s_Serializers[AssetType::EnvMap] = CreateScope<EnvironmentSerializer>();
		s_Serializers[AssetType::PhysicsMat] = CreateScope<PhysicsMaterialSerializer>();
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			HY_CORE_WARN("There's currently no importer for assets of type {0}", metadata.Extension);
			return;
		}

		s_Serializers[metadata.Type]->Serialize(metadata, asset);
	}

	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
	{
		if (metadata.Type == AssetType::Directory)
			return false;

		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			HY_CORE_WARN("There's currently no importer for assets of type {0}", metadata.Extension);
			return false;
		}

		return s_Serializers[metadata.Type]->TryLoadData(metadata, asset);
	}

	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;

}
