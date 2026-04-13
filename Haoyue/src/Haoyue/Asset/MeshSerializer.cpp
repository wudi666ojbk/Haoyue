#include "pch.h"
#include "MeshSerializer.h"

#include "yaml-cpp/yaml.h"

#include "Haoyue/Asset/AssetManager.h"

namespace Haoyue {

	MeshSerializer::MeshSerializer()
	{
	}

	bool MeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		// TODO: this needs to open up a Hazel Mesh file and make sure
		//       the MeshAsset file is also loaded
		HY_CORE_ASSERT(false);
		return false;
	}

	void MeshSerializer::Serialize(const std::string& filepath)
	{
	}

	void MeshSerializer::SerializeRuntime(const std::string& filepath)
	{
		HY_CORE_ASSERT(false);
	}

	bool MeshSerializer::Deserialize(const std::string& filepath)
	{
		return false;
	}

	bool MeshSerializer::DeserializeRuntime(const std::string& filepath)
	{
		HY_CORE_ASSERT(false);
		return false;
	}

}