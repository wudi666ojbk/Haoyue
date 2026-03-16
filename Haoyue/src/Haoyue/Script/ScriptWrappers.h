#pragma once

#include "Haoyue/Script/ScriptEngine.h"
#include "Haoyue/Core/Input.h"
#include "Haoyue/Physics/Physics.h"

#include <glm/glm.hpp>

extern "C" {
	typedef struct _MonoString MonoString;
	typedef struct _MonoArray MonoArray;
}

namespace Haoyue { namespace Script {

	// Math
	float Haoyue_Noise_PerlinNoise(float x, float y);

	// Input
	bool Haoyue_Input_IsKeyPressed(KeyCode key);
	bool Haoyue_Input_IsMouseButtonPressed(MouseButton button);
	void Haoyue_Input_GetMousePosition(glm::vec2* outPosition);
	void Haoyue_Input_SetCursorMode(CursorMode mode);
	CursorMode Haoyue_Input_GetCursorMode();

	// Physics
	bool Haoyue_Physics_Raycast(glm::vec3* origin, glm::vec3* direction, float maxDistance, RaycastHit* hit);
	MonoArray* Haoyue_Physics_OverlapBox(glm::vec3* origin, glm::vec3* halfSize);
	MonoArray* Haoyue_Physics_OverlapCapsule(glm::vec3* origin, float radius, float halfHeight);
	MonoArray* Haoyue_Physics_OverlapSphere(glm::vec3* origin, float radius);
	int32_t Haoyue_Physics_OverlapBoxNonAlloc(glm::vec3* origin, glm::vec3* halfSize, MonoArray* outColliders);
	int32_t Haoyue_Physics_OverlapCapsuleNonAlloc(glm::vec3* origin, float radius, float halfHeight, MonoArray* outColliders);
	int32_t Haoyue_Physics_OverlapSphereNonAlloc(glm::vec3* origin, float radius, MonoArray* outColliders);

	// Entity
	void Haoyue_Entity_CreateComponent(uint64_t entityID, void* type);
	bool Haoyue_Entity_HasComponent(uint64_t entityID, void* type);
	uint64_t Haoyue_Entity_FindEntityByTag(MonoString* tag);

	void Haoyue_TransformComponent_GetTransform(uint64_t entityID, TransformComponent* outTransform);
	void Haoyue_TransformComponent_SetTransform(uint64_t entityID, TransformComponent* inTransform);
	void Haoyue_TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* outTranslation);
	void Haoyue_TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* inTranslation);
	void Haoyue_TransformComponent_GetRotation(uint64_t entityID, glm::vec3* outRotation);
	void Haoyue_TransformComponent_SetRotation(uint64_t entityID, glm::vec3* inRotation);
	void Haoyue_TransformComponent_GetScale(uint64_t entityID, glm::vec3* outScale);
	void Haoyue_TransformComponent_SetScale(uint64_t entityID, glm::vec3* inScale);

	void* Haoyue_MeshComponent_GetMesh(uint64_t entityID);
	void Haoyue_MeshComponent_SetMesh(uint64_t entityID, Ref<Mesh>* inMesh);

	void Haoyue_RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake);
	void Haoyue_RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity);
	void Haoyue_RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* velocity);

	RigidBodyComponent::Type Haoyue_RigidBodyComponent_GetBodyType(uint64_t entityID);
	void Haoyue_RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* force, ForceMode foceMode);
	void Haoyue_RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* torque, ForceMode forceMode);
	void Haoyue_RigidBodyComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity);
	void Haoyue_RigidBodyComponent_SetLinearVelocity(uint64_t entityID, glm::vec3* velocity);
	void Haoyue_RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity);
	void Haoyue_RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* velocity);
	void Haoyue_RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* rotation);
	uint32_t Haoyue_RigidBodyComponent_GetLayer(uint64_t entityID);
	float Haoyue_RigidBodyComponent_GetMass(uint64_t entityID);
	void Haoyue_RigidBodyComponent_SetMass(uint64_t entityID, float mass);

	// Renderer
	// Texture2D
	void* Haoyue_Texture2D_Constructor(uint32_t width, uint32_t height);
	void Haoyue_Texture2D_Destructor(Ref<Texture2D>* _this);
	void Haoyue_Texture2D_SetData(Ref<Texture2D>* _this, MonoArray* inData, int32_t count);

	// Material
	void Haoyue_Material_Destructor(Ref<Material>* _this);
	void Haoyue_Material_SetFloat(Ref<Material>* _this, MonoString* uniform, float value);
	void Haoyue_Material_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture);

	void Haoyue_MaterialInstance_Destructor(Ref<Material>* _this);
	void Haoyue_MaterialInstance_SetFloat(Ref<Material>* _this, MonoString* uniform, float value);
	void Haoyue_MaterialInstance_SetVector3(Ref<Material>* _this, MonoString* uniform, glm::vec3* value);
	void Haoyue_MaterialInstance_SetVector4(Ref<Material>* _this, MonoString* uniform, glm::vec4* value);
	void Haoyue_MaterialInstance_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture);

	// Mesh
	Ref<Mesh>* Haoyue_Mesh_Constructor(MonoString* filepath);
	void Haoyue_Mesh_Destructor(Ref<Mesh>* _this);
	Ref<Material>* Haoyue_Mesh_GetMaterial(Ref<Mesh>* inMesh);
	Ref<Material>* Haoyue_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index);
	int Haoyue_Mesh_GetMaterialCount(Ref<Mesh>* inMesh);

	void* Haoyue_MeshFactory_CreatePlane(float width, float height);
} }