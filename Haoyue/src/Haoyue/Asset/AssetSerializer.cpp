#include "pch.h"
#include "AssetSerializer.h"
#include "Haoyue/Utilities/StringUtils.h"
#include "Haoyue/Utilities/FileSystem.h"
#include "Haoyue/Renderer/Mesh.h"
#include "Haoyue/Renderer/Renderer.h"

#include "yaml-cpp/yaml.h"

namespace Haoyue {

	bool TextureSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Texture2D::Create(metadata.FilePath);
		asset->Handle = metadata.Handle;

		bool result = asset.As<Texture2D>()->Loaded();

		if (!result)
			asset->SetFlag(AssetFlag::Invalid, true);

		return result;
	}

	bool MeshAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<Asset> temp = asset;
		asset = Ref<Mesh>::Create(metadata.FilePath);
		return (asset.As<Mesh>())->GetStaticVertices().size() > 0; // Maybe?
	}

	bool EnvironmentSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto [radiance, irradiance] = Renderer::CreateEnvironmentMap(metadata.FilePath);

		if (!radiance || !irradiance)
			return false;

		Ref<Asset> temp = asset;
		asset = Ref<Environment>::Create(radiance, irradiance);
		asset->Handle = metadata.Handle;
		return true;
	}

	void PhysicsMaterialSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<PhysicsMaterial> material = asset.As<PhysicsMaterial>();

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "StaticFriction" << material->StaticFriction;
		out << YAML::Key << "DynamicFriction" << material->DynamicFriction;
		out << YAML::Key << "Bounciness" << material->Bounciness;
		out << YAML::EndMap;

		std::ofstream fout(metadata.FilePath);
		fout << out.c_str();
	}

	bool PhysicsMaterialSerializer::TryLoadData(const AssetMetadata& metadata,Ref<Asset>& asset) const
	{
		std::ifstream stream(metadata.FilePath);
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		float staticFriction = data["StaticFriction"].as<float>();
		float dynamicFriction = data["DynamicFriction"].as<float>();
		float bounciness = data["Bounciness"].as<float>();

		asset = Ref<PhysicsMaterial>::Create(staticFriction, dynamicFriction, bounciness);
		asset->Handle = metadata.Handle;
		return true;
	}

}