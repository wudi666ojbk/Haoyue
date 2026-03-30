#include "pch.h"
#include "ScriptEngineRegistry.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Haoyue/Scene/Entity.h"
#include "ScriptWrappers.h"

namespace Haoyue {

	std::unordered_map<MonoType*, std::function<bool(Entity&)>> s_HasComponentFuncs;
	std::unordered_map<MonoType*, std::function<void(Entity&)>> s_CreateComponentFuncs;

	extern MonoImage* s_CoreAssemblyImage;

#define Component_RegisterType(Type) \
	{\
		MonoType* type = mono_reflection_type_from_name("Haoyue." #Type, s_CoreAssemblyImage);\
		if (type) {\
			uint32_t id = mono_type_get_type(type);\
			s_HasComponentFuncs[type] = [](Entity& entity) { return entity.HasComponent<Type>(); };\
			s_CreateComponentFuncs[type] = [](Entity& entity) { entity.AddComponent<Type>(); };\
		} else {\
			HY_CORE_ERROR("No C# component class found for " #Type "!");\
		}\
	}

	static void InitComponentTypes()
	{
		Component_RegisterType(TagComponent);
		Component_RegisterType(TransformComponent);
		Component_RegisterType(MeshComponent);
		Component_RegisterType(ScriptComponent);
		Component_RegisterType(CameraComponent);
		Component_RegisterType(SpriteRendererComponent);
		Component_RegisterType(RigidBody2DComponent);
		Component_RegisterType(BoxCollider2DComponent);
		Component_RegisterType(RigidBodyComponent);
		Component_RegisterType(BoxColliderComponent);
		Component_RegisterType(SphereColliderComponent);
	}

	void ScriptEngineRegistry::RegisterAll()
	{
		InitComponentTypes();

		mono_add_internal_call("Haoyue.Noise::PerlinNoise_Native", Haoyue::Script::Haoyue_Noise_PerlinNoise);

		mono_add_internal_call("Haoyue.Physics::Raycast_Native", Haoyue::Script::Haoyue_Physics_Raycast);
		mono_add_internal_call("Haoyue.Physics::OverlapBox_Native", Haoyue::Script::Haoyue_Physics_OverlapBox);
		mono_add_internal_call("Haoyue.Physics::OverlapCapsule_Native", Haoyue::Script::Haoyue_Physics_OverlapCapsule);
		mono_add_internal_call("Haoyue.Physics::OverlapSphere_Native", Haoyue::Script::Haoyue_Physics_OverlapSphere);
		mono_add_internal_call("Haoyue.Physics::OverlapBoxNonAlloc_Native", Haoyue::Script::Haoyue_Physics_OverlapBoxNonAlloc);
		mono_add_internal_call("Haoyue.Physics::OverlapCapsuleNonAlloc_Native", Haoyue::Script::Haoyue_Physics_OverlapCapsuleNonAlloc);
		mono_add_internal_call("Haoyue.Physics::OverlapSphereNonAlloc_Native", Haoyue::Script::Haoyue_Physics_OverlapSphereNonAlloc);

		mono_add_internal_call("Haoyue.Entity::CreateComponent_Native", Haoyue::Script::Haoyue_Entity_CreateComponent);
		mono_add_internal_call("Haoyue.Entity::HasComponent_Native", Haoyue::Script::Haoyue_Entity_HasComponent);
		mono_add_internal_call("Haoyue.Entity::FindEntityByTag_Native", Haoyue::Script::Haoyue_Entity_FindEntityByTag);

		mono_add_internal_call("Haoyue.TransformComponent::GetTransform_Native", Haoyue::Script::Haoyue_TransformComponent_GetTransform);
		mono_add_internal_call("Haoyue.TransformComponent::SetTransform_Native", Haoyue::Script::Haoyue_TransformComponent_SetTransform);
		mono_add_internal_call("Haoyue.TransformComponent::GetTranslation_Native", Haoyue::Script::Haoyue_TransformComponent_GetTranslation);
		mono_add_internal_call("Haoyue.TransformComponent::SetTranslation_Native", Haoyue::Script::Haoyue_TransformComponent_SetTranslation);
		mono_add_internal_call("Haoyue.TransformComponent::GetRotation_Native", Haoyue::Script::Haoyue_TransformComponent_GetRotation);
		mono_add_internal_call("Haoyue.TransformComponent::SetRotation_Native", Haoyue::Script::Haoyue_TransformComponent_SetRotation);
		mono_add_internal_call("Haoyue.TransformComponent::GetScale_Native", Haoyue::Script::Haoyue_TransformComponent_GetScale);
		mono_add_internal_call("Haoyue.TransformComponent::SetScale_Native", Haoyue::Script::Haoyue_TransformComponent_SetScale);
		mono_add_internal_call("Hazel.TransformComponent::GetWorldTranslation_Native", Haoyue::Script::Haoyue_TransformComponent_GetWorldTranslation);

		mono_add_internal_call("Haoyue.MeshComponent::GetMesh_Native", Haoyue::Script::Haoyue_MeshComponent_GetMesh);
		mono_add_internal_call("Haoyue.MeshComponent::SetMesh_Native", Haoyue::Script::Haoyue_MeshComponent_SetMesh);

		mono_add_internal_call("Haoyue.RigidBody2DComponent::ApplyLinearImpulse_Native", Haoyue::Script::Haoyue_RigidBody2DComponent_ApplyLinearImpulse);
		mono_add_internal_call("Haoyue.RigidBody2DComponent::GetLinearVelocity_Native", Haoyue::Script::Haoyue_RigidBody2DComponent_GetLinearVelocity);
		mono_add_internal_call("Haoyue.RigidBody2DComponent::SetLinearVelocity_Native", Haoyue::Script::Haoyue_RigidBody2DComponent_SetLinearVelocity);

		mono_add_internal_call("Haoyue.RigidBodyComponent::GetBodyType_Native", Haoyue::Script::Haoyue_RigidBodyComponent_GetBodyType);
		mono_add_internal_call("Haoyue.RigidBodyComponent::AddForce_Native", Haoyue::Script::Haoyue_RigidBodyComponent_AddForce);
		mono_add_internal_call("Haoyue.RigidBodyComponent::AddTorque_Native", Haoyue::Script::Haoyue_RigidBodyComponent_AddTorque);
		mono_add_internal_call("Haoyue.RigidBodyComponent::GetLinearVelocity_Native", Haoyue::Script::Haoyue_RigidBodyComponent_GetLinearVelocity);
		mono_add_internal_call("Haoyue.RigidBodyComponent::SetLinearVelocity_Native", Haoyue::Script::Haoyue_RigidBodyComponent_SetLinearVelocity);
		mono_add_internal_call("Haoyue.RigidBodyComponent::GetAngularVelocity_Native", Haoyue::Script::Haoyue_RigidBodyComponent_GetAngularVelocity);
		mono_add_internal_call("Haoyue.RigidBodyComponent::SetAngularVelocity_Native", Haoyue::Script::Haoyue_RigidBodyComponent_SetAngularVelocity);
		mono_add_internal_call("Haoyue.RigidBodyComponent::Rotate_Native", Haoyue::Script::Haoyue_RigidBodyComponent_Rotate);
		mono_add_internal_call("Haoyue.RigidBodyComponent::GetLayer_Native", Haoyue::Script::Haoyue_RigidBodyComponent_GetLayer);
		mono_add_internal_call("Haoyue.RigidBodyComponent::GetMass_Native", Haoyue::Script::Haoyue_RigidBodyComponent_GetMass);
		mono_add_internal_call("Haoyue.RigidBodyComponent::SetMass_Native", Haoyue::Script::Haoyue_RigidBodyComponent_SetMass);

		mono_add_internal_call("Haoyue.Input::IsKeyPressed_Native", Haoyue::Script::Haoyue_Input_IsKeyPressed);
		mono_add_internal_call("Haoyue.Input::IsMouseButtonPressed_Native", Haoyue::Script::Haoyue_Input_IsMouseButtonPressed);
		mono_add_internal_call("Haoyue.Input::GetMousePosition_Native", Haoyue::Script::Haoyue_Input_GetMousePosition);
		mono_add_internal_call("Haoyue.Input::SetCursorMode_Native", Haoyue::Script::Haoyue_Input_SetCursorMode);
		mono_add_internal_call("Haoyue.Input::GetCursorMode_Native", Haoyue::Script::Haoyue_Input_GetCursorMode);

		mono_add_internal_call("Haoyue.Texture2D::Constructor_Native", Haoyue::Script::Haoyue_Texture2D_Constructor);
		mono_add_internal_call("Haoyue.Texture2D::Destructor_Native", Haoyue::Script::Haoyue_Texture2D_Destructor);
		mono_add_internal_call("Haoyue.Texture2D::SetData_Native", Haoyue::Script::Haoyue_Texture2D_SetData);

		mono_add_internal_call("Haoyue.Material::Destructor_Native", Haoyue::Script::Haoyue_Material_Destructor);
		mono_add_internal_call("Haoyue.Material::SetFloat_Native", Haoyue::Script::Haoyue_Material_SetFloat);
		mono_add_internal_call("Haoyue.Material::SetTexture_Native", Haoyue::Script::Haoyue_Material_SetTexture);

		mono_add_internal_call("Haoyue.MaterialInstance::Destructor_Native", Haoyue::Script::Haoyue_MaterialInstance_Destructor);
		mono_add_internal_call("Haoyue.MaterialInstance::SetFloat_Native", Haoyue::Script::Haoyue_MaterialInstance_SetFloat);
		mono_add_internal_call("Haoyue.MaterialInstance::SetVector3_Native", Haoyue::Script::Haoyue_MaterialInstance_SetVector3);
		mono_add_internal_call("Haoyue.MaterialInstance::SetVector4_Native", Haoyue::Script::Haoyue_MaterialInstance_SetVector4);
		mono_add_internal_call("Haoyue.MaterialInstance::SetTexture_Native", Haoyue::Script::Haoyue_MaterialInstance_SetTexture);

		mono_add_internal_call("Haoyue.Mesh::Constructor_Native", Haoyue::Script::Haoyue_Mesh_Constructor);
		mono_add_internal_call("Haoyue.Mesh::Destructor_Native", Haoyue::Script::Haoyue_Mesh_Destructor);
		mono_add_internal_call("Haoyue.Mesh::GetMaterial_Native", Haoyue::Script::Haoyue_Mesh_GetMaterial);
		mono_add_internal_call("Haoyue.Mesh::GetMaterialByIndex_Native", Haoyue::Script::Haoyue_Mesh_GetMaterialByIndex);
		mono_add_internal_call("Haoyue.Mesh::GetMaterialCount_Native", Haoyue::Script::Haoyue_Mesh_GetMaterialCount);

		mono_add_internal_call("Haoyue.MeshFactory::CreatePlane_Native", Haoyue::Script::Haoyue_MeshFactory_CreatePlane);
	}

}