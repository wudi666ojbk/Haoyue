#pragma once

#include "Haoyue/Asset/AssetTypes.h"

#include "Haoyue/Core/UUID.h"

#include <entt/entt.hpp>

namespace Haoyue {

	using AssetHandle = UUID;

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle;
		AssetType Type = AssetType::None;

		std::string FilePath;
		std::string FileName;
		std::string Extension;
		AssetHandle ParentDirectory;
		bool IsDataLoaded = false;

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}
		
		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}
		virtual ~Asset() {}
	};

	class PhysicsMaterial : public Asset
	{
	public:
		float StaticFriction;
		float DynamicFriction;
		float Bounciness;

		PhysicsMaterial() = default;
		PhysicsMaterial(float staticFriction, float dynamicFriction, float bounciness)
			: StaticFriction(staticFriction), DynamicFriction(dynamicFriction), Bounciness(bounciness)
		{
		}
	};

	// Treating directories as assets simplifies the asset manager window rendering by a lot
	class Directory : public Asset
	{
	public:
		std::vector<AssetHandle> ChildDirectories;

		Directory() = default;
	};
}
