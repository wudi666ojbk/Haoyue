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
		uint16_t Flags = (uint16_t)AssetFlag::None;

		virtual ~Asset() {}

		virtual AssetType GetAssetType() const { return AssetType::None; }
		static AssetType GetStaticType() { return AssetType::None; }

		bool IsValid() const { return ((Flags & (uint16_t)AssetFlag::Missing) | (Flags & (uint16_t)AssetFlag::Invalid)) == 0; }

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}
		
		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}
		
		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlag flag, bool value)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}
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

		static AssetType GetStaticType() { return AssetType::PhysicsMat; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

	// Treating directories as assets simplifies the asset manager window rendering by a lot
	class Directory : public Asset
	{
	public:
		std::vector<AssetHandle> ChildDirectories;

		Directory() = default;

		static AssetType GetStaticType() { return AssetType::Directory; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};
}
