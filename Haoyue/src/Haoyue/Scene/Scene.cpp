#include "pch.h"
#include "Scene.h"

#include "Entity.h"

#include "Components.h"

#include "Haoyue/Renderer/SceneRenderer.h"
#include "Haoyue/Script/ScriptEngine.h"

#include "Haoyue/Renderer/Renderer2D.h"
#include "Haoyue/Physics/Physics.h"
#include "Haoyue/Physics/PhysicsActor.h"
#include "Haoyue/Audio/AudioEngine.h"
#include "Haoyue/Audio/AudioComponent.h"

#include "Haoyue/Math/Math.h"
#include "Haoyue/Renderer/Renderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

// Box2D
#include <box2d/box2d.h>

// TEMP
#include "Haoyue/Core/Input.h"

namespace Haoyue {

	static const std::string DefaultEntityName = "Entity";

	std::unordered_map<UUID, Scene*> s_ActiveScenes;

	struct SceneComponent
	{
		UUID SceneID;
	};

	// TODO: MOVE TO PHYSICS FILE!
	class ContactListener2D : public b2ContactListener
	{
	public:
		virtual void BeginContact(b2Contact* contact) override
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData();
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData();

			// TODO: improve these if checks
			if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
				ScriptEngine::OnCollision2DBegin(a);

			if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName))
				ScriptEngine::OnCollision2DBegin(b);
		}

		/// Called when two fixtures cease to touch.
		virtual void EndContact(b2Contact* contact) override
		{
			Entity& a = *(Entity*)contact->GetFixtureA()->GetBody()->GetUserData();
			Entity& b = *(Entity*)contact->GetFixtureB()->GetBody()->GetUserData();

			// TODO: improve these if checks
			if (a.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(a.GetComponent<ScriptComponent>().ModuleName))
				ScriptEngine::OnCollision2DEnd(a);

			if (b.HasComponent<ScriptComponent>() && ScriptEngine::ModuleExists(b.GetComponent<ScriptComponent>().ModuleName))
				ScriptEngine::OnCollision2DEnd(b);
		}

		/// 在更新接触后调用此函数。通过它，你可以在接触信息传递给求解器之前对其进行检查。若操作得当，你可以修改接触流形（例如禁用接触）。
		/// 此处会提供旧流形的副本，以便你检测其变化。
		/// 注意：此函数仅在刚体处于唤醒状态时调用。
		/// 注意：即使接触点数量为零，此函数也会被调用。
		/// 注意：传感器不会触发此函数。
		/// 注意：若你将接触点数量设为零，则不会收到EndContact回调。但可能在下一步收到BeginContact回调。
		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override
		{
			B2_NOT_USED(contact);
			B2_NOT_USED(oldManifold);
		}

		/// 在求解器完成计算后调用此函数，通过它你可以检查接触信息。这对于查看冲量非常有用。
		/// 注意：接触流形不包含碰撞时间产生的冲量，若子步长较小，该冲量可能任意大。因此，冲量会在单独的数据结构中明确提供。
		/// 注意：此函数仅针对正在接触、为实体且处于唤醒状态的接触进行调用。
		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override
		{
			B2_NOT_USED(contact);
			B2_NOT_USED(impulse);
		}
	};

	static ContactListener2D s_Box2DContactListener;

	struct Box2DWorldComponent
	{
		std::unique_ptr<b2World> World;
	};

	static void OnScriptComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		Scene* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;
		HY_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		ScriptEngine::InitScriptEntity(scene->m_EntityIDMap.at(entityID));
	}

	static void OnScriptComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		Scene* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;
		ScriptEngine::OnScriptComponentDestroyed(sceneID, entityID);
	}

	static void OnAudioComponentConstruct(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		Scene* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;
		HY_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		registry.get<Audio::AudioComponent>(entity).ParentHandle = entityID;
		Audio::MiniAudioEngine::Get().RegisterAudioComponent(scene->m_EntityIDMap.at(entityID));
	}

	//? This just throws that entity does not exist when looking for IDComponent, so it can't be use reliably
	static void OnAudioComponentDestroy(entt::registry& registry, entt::entity entity)
	{
		auto sceneView = registry.view<SceneComponent>();
		UUID sceneID = registry.get<SceneComponent>(sceneView.front()).SceneID;

		Scene* scene = s_ActiveScenes[sceneID];

		auto entityID = registry.get<IDComponent>(entity).ID;
		HY_CORE_ASSERT(scene->m_EntityIDMap.find(entityID) != scene->m_EntityIDMap.end());
		Audio::MiniAudioEngine::Get().UnregisterAudioComponent(sceneID, scene->m_EntityIDMap.at(entityID).GetUUID());
	}

	Scene::Scene(const std::string& debugName, bool isEditorScene)
		: m_DebugName(debugName), m_IsEditorScene(isEditorScene)
	{
		m_Registry.on_construct<ScriptComponent>().connect<&OnScriptComponentConstruct>();
		m_Registry.on_destroy<ScriptComponent>().connect<&OnScriptComponentDestroy>();
		m_Registry.on_construct<Audio::AudioComponent>().connect<&OnAudioComponentConstruct>();

		m_SceneEntity = m_Registry.create();
		m_Registry.emplace<SceneComponent>(m_SceneEntity, m_SceneID);

		// TODO: Obviously not necessary in all cases
		Box2DWorldComponent& b2dWorld = m_Registry.emplace<Box2DWorldComponent>(m_SceneEntity, std::make_unique<b2World>(b2Vec2{ 0.0f, -9.8f }));
		b2dWorld.World->SetContactListener(&s_Box2DContactListener);

		s_ActiveScenes[m_SceneID] = this;

		if (!isEditorScene)
			Physics::CreateScene();

		Init();
	}

	Scene::~Scene()
	{
		m_Registry.on_destroy<ScriptComponent>().disconnect();

		m_Registry.clear();
		s_ActiveScenes.erase(m_SceneID);
		ScriptEngine::OnSceneDestruct(m_SceneID);
		Audio::MiniAudioEngine::OnSceneDestruct(m_SceneID);
	}

	void Scene::Init()
	{
		auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");
		m_SkyboxMaterial = Material::Create(skyboxShader);
		m_SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, false);
	}

	void Scene::OnUpdate(Timestep ts)
	{
		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& box2DWorld = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;
		int32_t velocityIterations = 6;
		int32_t positionIterations = 2;
		box2DWorld->Step(ts, velocityIterations, positionIterations);

		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& rb2d = e.GetComponent<RigidBody2DComponent>();
				b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);

				auto& position = body->GetPosition();
				auto& transform = e.GetComponent<TransformComponent>();
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			}
		}

		// Update all entities
		{
			auto view = m_Registry.view<ScriptComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
					ScriptEngine::OnUpdateEntity(e, ts);
			}
		}

		{
			auto view = m_Registry.view<TransformComponent>();
			for (auto entity : view)
			{
				auto& transformComponent = view.get(entity);
				Entity e = Entity(entity, this);
				glm::mat4 transform = GetTransformRelativeToParent(e);
				glm::vec3 translation;
				glm::vec3 rotation;
				glm::vec3 scale;
				Math::DecomposeTransform(transform, translation, rotation, scale);

				glm::quat rotationQuat = glm::quat(rotation);
				transformComponent.Up = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 1.0f, 0.0f)));
				transformComponent.Right = glm::normalize(glm::rotate(rotationQuat, glm::vec3(1.0f, 0.0f, 0.0f)));
				transformComponent.Forward = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 0.0f, -1.0f)));
			}
		}

		{	//--- Update Audio Components ---

			auto view = m_Registry.view<Audio::AudioComponent>();

			std::vector<Entity> deadEntities;
			deadEntities.reserve(view.size());

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& audioComponent = e.GetComponent<Audio::AudioComponent>();

				// 1. Handle Audio Components marked for Auto Destroy

				// AutoDestroy flag is only set for "one-shot" sounds
				if (audioComponent.bAutoDestroy && audioComponent.bMarkedForDestroy)
				{
					deadEntities.push_back(e);
					continue;
				}

				// 2. Update positions of associated sound sources

				auto& worldSpaceTransform = GetWorldSpaceTransform(e);

				// 3. Update velocities of associated sound sources
				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				if (auto physicsActor = Physics::GetActorForEntity(e))
				{
					if (physicsActor->IsDynamic())
						velocity = physicsActor->GetLinearVelocity();
				}

				updateData.emplace_back(SoundSourceUpdateData{ e.GetUUID(),
					audioComponent.VolumeMultiplier,
					audioComponent.PitchMultiplier,
					worldSpaceTransform.Translation,
					velocity });
			}

			//--- Submit values to AudioEngine to update associated sound sources ---
			//-----------------------------------------------------------------------
			Audio::MiniAudioEngine::Get().SubmitSourceUpdateData(updateData);

			for (int i = deadEntities.size() - 1; i >= 0; i--)
			{
				DestroyEntity(deadEntities[i]);
			}
		}

		{	//--- Update Audio Listener ---
			auto view = m_Registry.view<AudioListenerComponent>();
			Entity listener;
			for (auto entity : view)
			{
				Entity e = { entity, this };
				if (e.GetComponent<AudioListenerComponent>().Active)
				{
					listener = e;
					auto worldSpaceTransform = GetWorldSpaceTransform(listener);
					Audio::MiniAudioEngine::Get().UpdateListenerPosition(worldSpaceTransform.Translation, worldSpaceTransform.Forward);

					if (auto physicsActor = Physics::GetActorForEntity(listener))
					{
						if (physicsActor->IsDynamic())
							Audio::MiniAudioEngine::Get().UpdateListenerVelocity(physicsActor->GetLinearVelocity());
					}
					break;
				}
			}

			// If listener wasn't found, fallback to using main camera as an active listener
			if (listener.m_EntityHandle == entt::null)
			{
				listener = GetMainCameraEntity();
				if (listener.m_EntityHandle != entt::null)
				{
					// If camera was changed or destroyed during Runtime, it might not have Listener Component (?)
					if (!listener.HasComponent<AudioListenerComponent>())
						listener.AddComponent<AudioListenerComponent>();

					auto worldSpaceTransform = GetWorldSpaceTransform(listener);
					Audio::MiniAudioEngine::Get().UpdateListenerPosition(worldSpaceTransform.Translation, worldSpaceTransform.Forward);

					if (auto physicsActor = Physics::GetActorForEntity(listener))
					{
						if (physicsActor->IsDynamic())
							Audio::MiniAudioEngine::Get().UpdateListenerVelocity(physicsActor->GetLinearVelocity());
					}
				}
			}
		}

		Physics::Simulate(ts);
	}

	void Scene::OnRenderRuntime(Ref<SceneRenderer> renderer, Timestep ts)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////
		Entity cameraEntity = GetMainCameraEntity();
		if (!cameraEntity)
			return;

		glm::mat4 cameraViewMatrix = glm::inverse(GetTransformRelativeToParent(cameraEntity));
		HY_CORE_ASSERT(cameraEntity, "Scene does not contain any cameras!");
		SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>();
		camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);

		// Process lights
		{
			m_LightEnvironment = LightEnvironment();
			auto lights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
			uint32_t directionalLightIndex = 0;
			for (auto entity : lights)
			{
				auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);
				glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
				m_LightEnvironment.DirectionalLights[directionalLightIndex++] =
				{
					direction,
					lightComponent.Radiance,
					lightComponent.Intensity,
					lightComponent.CastShadows
				};
			}
		}

		// TODO: only one sky light at the moment!
		{
			//m_Environment = Ref<Environment>::Create();
			auto lights = m_Registry.group<SkyLightComponent>(entt::get<TransformComponent>);
			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
				m_Environment = skyLightComponent.SceneEnvironment;
				m_EnvironmentIntensity = skyLightComponent.Intensity;
				if (m_Environment)
					SetSkybox(m_Environment->RadianceMap);
			}
		}

		m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

		auto group = m_Registry.group<MeshComponent>(entt::get<TransformComponent>);
		renderer->SetScene(this);
		renderer->BeginScene({ camera, cameraViewMatrix });
		for (auto entity : group)
		{
			auto [transformComponent, meshComponent] = group.get<TransformComponent, MeshComponent>(entity);
			if (meshComponent.Mesh && !meshComponent.Mesh->IsFlagSet(AssetFlag::Missing))
			{
				meshComponent.Mesh->OnUpdate(ts);
				glm::mat4 transform = GetTransformRelativeToParent(Entity(entity, this));

				// TODO: Should we render (logically)
				renderer->SubmitMesh(meshComponent, transform);
			}
		}
		renderer->EndScene();
	}

	void Scene::OnRenderEditor(Ref<SceneRenderer> renderer, Timestep ts, const EditorCamera& editorCamera)
	{
		/////////////////////////////////////////////////////////////////////
		// RENDER 3D SCENE
		/////////////////////////////////////////////////////////////////////

		{
			m_LightEnvironment = LightEnvironment();
			auto lights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
			uint32_t directionalLightIndex = 0;
			for (auto entity : lights)
			{
				auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);
				glm::vec3 direction = -glm::normalize(glm::mat3(transformComponent.GetTransform()) * glm::vec3(1.0f));
				m_LightEnvironment.DirectionalLights[directionalLightIndex++] =
				{
					direction,
					lightComponent.Radiance,
					lightComponent.Intensity,
					lightComponent.CastShadows
				};
			}
		}

		{
			auto lights = m_Registry.group<SkyLightComponent>(entt::get<TransformComponent>);
			if (lights.empty())
				m_Environment = Ref<Environment>::Create(Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture());

			for (auto entity : lights)
			{
				auto [transformComponent, skyLightComponent] = lights.get<TransformComponent, SkyLightComponent>(entity);
				if (!skyLightComponent.SceneEnvironment && skyLightComponent.DynamicSky)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(skyLightComponent.TurbidityAzimuthInclination.x, skyLightComponent.TurbidityAzimuthInclination.y, skyLightComponent.TurbidityAzimuthInclination.z);
					skyLightComponent.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
				}
				m_Environment = skyLightComponent.SceneEnvironment;
				m_EnvironmentIntensity = skyLightComponent.Intensity;
				if (m_Environment)
					SetSkybox(m_Environment->RadianceMap);
			}
		}

		m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SkyboxLod);

		auto group = m_Registry.group<MeshComponent>(entt::get<TransformComponent>);
		renderer->SetScene(this);
		renderer->BeginScene({ editorCamera, editorCamera.GetViewMatrix(), 0.1f, 1000.0f, 45.0f }); // TODO: real values
		for (auto entity : group)
		{
			auto [meshComponent, transformComponent] = group.get<MeshComponent, TransformComponent>(entity);
			if (meshComponent.Mesh && !meshComponent.Mesh->IsFlagSet(AssetFlag::Missing))
			{
				meshComponent.Mesh->OnUpdate(ts);

				// TODO(Peter): Is this any good?
				glm::mat4 transform = GetTransformRelativeToParent(Entity{ entity, this });

				// TODO: Should we render (logically)
				if (m_SelectedEntity == entity)
					renderer->SubmitSelectedMesh(meshComponent, transform);
				else
					renderer->SubmitMesh(meshComponent, transform);
			}
		}

		{
			auto view = m_Registry.view<BoxColliderComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				glm::mat4 transform = GetTransformRelativeToParent(e);
				auto& collider = e.GetComponent<BoxColliderComponent>();

				if (m_SelectedEntity == entity)
					renderer->SubmitColliderMesh(collider, transform);
			}
		}

		{
			auto view = m_Registry.view<SphereColliderComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				glm::mat4 transform = GetTransformRelativeToParent(e);
				auto& collider = e.GetComponent<SphereColliderComponent>();

				if (m_SelectedEntity == entity)
					renderer->SubmitColliderMesh(collider, transform);
			}
		}

		{
			auto view = m_Registry.view<CapsuleColliderComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				glm::mat4 transform = GetTransformRelativeToParent(e);
				auto& collider = e.GetComponent<CapsuleColliderComponent>();

				if (m_SelectedEntity == entity)
					renderer->SubmitColliderMesh(collider, transform);
			}
		}

		{
			auto view = m_Registry.view<MeshColliderComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				glm::mat4 transform = GetTransformRelativeToParent(e);
				auto& collider = e.GetComponent<MeshColliderComponent>();

				if (m_SelectedEntity == entity)
					renderer->SubmitColliderMesh(collider, transform);
			}
		}


		renderer->EndScene();

		{
			const auto& camPosition = editorCamera.GetPosition();
			auto camDirection = glm::rotate(editorCamera.GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
			Audio::MiniAudioEngine::Get().UpdateListenerPosition(camPosition, camDirection);
		}

		//--- Update Audio Component positions (editor scene update) ---
		{
			auto view = m_Registry.view<Audio::AudioComponent>();

			std::vector<Entity> deadEntities;

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& audioComponent = e.GetComponent<Audio::AudioComponent>();

				// AutoDestroy 标志仅针对"一次性"音效设置
				if (audioComponent.bAutoDestroy && audioComponent.bMarkedForDestroy)
				{
					deadEntities.push_back(e);
					continue;
				}

				auto& worldSpaceTransform = GetWorldSpaceTransform(e);

				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				updateData.emplace_back(SoundSourceUpdateData{ e.GetUUID(),
					audioComponent.VolumeMultiplier,
					audioComponent.PitchMultiplier,
					worldSpaceTransform.Translation,
					velocity });
			}

			//--- 向音频引擎提交数据以更新关联的声源 ---
			Audio::MiniAudioEngine::Get().SubmitSourceUpdateData(updateData);

			for (int i = deadEntities.size() - 1; i >= 0; i--)
			{
				DestroyEntity(deadEntities[i]);
			}
		}
	}

	void Scene::OnEvent(Event& e)
	{
	}

	void Scene::OnRuntimeStart()
	{
		ScriptEngine::SetSceneContext(this);
		Audio::MiniAudioEngine::SetSceneContext(this);

		{
			auto view = m_Registry.view<ScriptComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				if (ScriptEngine::ModuleExists(e.GetComponent<ScriptComponent>().ModuleName))
					ScriptEngine::InstantiateEntityClass(e);
			}
		}

		// Box2D physics
		auto sceneView = m_Registry.view<Box2DWorldComponent>();
		auto& world = m_Registry.get<Box2DWorldComponent>(sceneView.front()).World;
		
		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			m_Physics2DBodyEntityBuffer = new Entity[view.size()];
			uint32_t physicsBodyEntityBufferIndex = 0;
			for (auto entity : view)
			{
				Entity e = { entity, this };
				UUID entityID = e.GetComponent<IDComponent>().ID;
				TransformComponent& transform = e.GetComponent<TransformComponent>();
				auto& rigidBody2D = m_Registry.get<RigidBody2DComponent>(entity);

				b2BodyDef bodyDef;
				if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Static)
					bodyDef.type = b2_staticBody;
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Dynamic)
					bodyDef.type = b2_dynamicBody;
				else if (rigidBody2D.BodyType == RigidBody2DComponent::Type::Kinematic)
					bodyDef.type = b2_kinematicBody;
				bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
				
				bodyDef.angle = transform.Rotation.z;

				b2Body* body = world->CreateBody(&bodyDef);
				body->SetFixedRotation(rigidBody2D.FixedRotation);
				Entity* entityStorage = &m_Physics2DBodyEntityBuffer[physicsBodyEntityBufferIndex++];
				*entityStorage = e;
				body->SetUserData((void*)entityStorage);
				rigidBody2D.RuntimeBody = body;
			}
		}

		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& boxCollider2D = m_Registry.get<BoxCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					HY_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2PolygonShape polygonShape;
					polygonShape.SetAsBox(boxCollider2D.Size.x, boxCollider2D.Size.y);

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &polygonShape;
					fixtureDef.density = boxCollider2D.Density;
					fixtureDef.friction = boxCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		{
			auto view = m_Registry.view<CircleCollider2DComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				auto& transform = e.Transform();

				auto& circleCollider2D = m_Registry.get<CircleCollider2DComponent>(entity);
				if (e.HasComponent<RigidBody2DComponent>())
				{
					auto& rigidBody2D = e.GetComponent<RigidBody2DComponent>();
					HY_CORE_ASSERT(rigidBody2D.RuntimeBody);
					b2Body* body = static_cast<b2Body*>(rigidBody2D.RuntimeBody);

					b2CircleShape circleShape;
					circleShape.m_radius = circleCollider2D.Radius;

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &circleShape;
					fixtureDef.density = circleCollider2D.Density;
					fixtureDef.friction = circleCollider2D.Friction;
					body->CreateFixture(&fixtureDef);
				}
			}
		}

		// If the entity doesn't have a rigidbody but has a collider, give it a default rigidbody
		{
			auto view = m_Registry.view<TransformComponent>(entt::exclude<RigidBodyComponent>);
			for (auto entity : view)
			{
				if (m_Registry.any<BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, MeshColliderComponent>(entity))
				{
					Entity e = { entity, this };
					if (!e.HasComponent<RigidBodyComponent>())
						e.AddComponent<RigidBodyComponent>();
				}
			}
		}

		{
			auto view = m_Registry.view<RigidBodyComponent>();
			for (auto entity : view)
			{
				Entity e = { entity, this };
				Physics::CreateActor(e);
			}
		}

		{	//--- Make sure we have an audio listener ---
			//===========================================

			// If no audio listeners were added by the user, create one on the main camera

			// TODO: make a user option to automatically set active listener to the main camera vs override

			// Main Camera should have listener component in case of fallback
			Entity mainCam = GetMainCameraEntity();
			Entity listener;

			auto view = m_Registry.view<AudioListenerComponent>();
			bool listenerFound = !view.empty();

			if (mainCam.m_EntityHandle != entt::null)
			{
				if (!mainCam.HasComponent<AudioListenerComponent>())
					mainCam.AddComponent<AudioListenerComponent>();
			}

			if (listenerFound)
			{
				for (auto entity : view)
				{
					listener = { entity, this };
					if (listener.GetComponent<AudioListenerComponent>().Active)
					{
						listenerFound = true;
						break;
					}
					listenerFound = false;
				}
			}

			// If found listener has not been set Active, fallback to using Main Camera
			if (!listenerFound)
				listener = mainCam;

			// Don't update position if we faild to get active listener and Main Camera
			if (listener.m_EntityHandle != entt::null)
			{
				// Initialize listener's position
				auto& worldSpaceTransform = GetWorldSpaceTransform(listener);
				Audio::MiniAudioEngine::Get().UpdateListenerPosition(worldSpaceTransform.Translation, worldSpaceTransform.Forward);
			}
		}

		{	//--- Initialize audio component sound positions ---
			auto view = m_Registry.view<Audio::AudioComponent>();

			std::vector<SoundSourceUpdateData> updateData;
			updateData.reserve(view.size());

			for (auto entity : view)
			{
				auto& audioComponent = view.get(entity);

				Entity e = { entity, this };
				auto& worldSpaceTransform = GetWorldSpaceTransform(e);

				// If sounds are not spawned yet, this sets "spawn" position
				audioComponent.SourcePosition = worldSpaceTransform.Translation;


				glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
				updateData.emplace_back(SoundSourceUpdateData{ e.GetUUID(),
					audioComponent.VolumeMultiplier,
					audioComponent.PitchMultiplier,
					worldSpaceTransform.Translation,
					velocity });
			}

			//--- Submit values to AudioEngine to update associated sound sources ---
			Audio::MiniAudioEngine::Get().SubmitSourceUpdateData(updateData);
		}

		m_IsPlaying = true;

		Audio::MiniAudioEngine::OnRuntimePlaying(m_SceneID);
	}

	void Scene::OnRuntimeStop()
	{
		Input::SetCursorMode(CursorMode::Normal);

		delete[] m_Physics2DBodyEntityBuffer;
		Physics::DestroyScene();
		m_IsPlaying = false;
	}

	void Scene::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	void Scene::SetSkybox(const Ref<TextureCube>& skybox)
	{
		//m_SkyboxTexture = skybox;
		//m_SkyboxMaterial->Set("u_Texture", skybox);
	}

	Entity Scene::GetMainCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& comp = view.get<CameraComponent>(entity);
			if (comp.Primary)
				return { entity, this };
		}
		return {};
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		auto entity = Entity{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = {};

		entity.AddComponent<TransformComponent>();
		if (!name.empty())
			entity.AddComponent<TagComponent>(name);

		entity.AddComponent<RelationshipComponent>();

		m_EntityIDMap[idComponent.ID] = entity;
		return entity;
	}

	Entity Scene::CreateEntityWithID(UUID uuid, const std::string& name, bool runtimeMap)
	{
		auto entity = Entity{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = uuid;

		entity.AddComponent<TransformComponent>();
		if (!name.empty())
			entity.AddComponent<TagComponent>(name);

		entity.AddComponent<RelationshipComponent>();

		HY_CORE_ASSERT(m_EntityIDMap.find(uuid) == m_EntityIDMap.end());
		m_EntityIDMap[uuid] = entity;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		if (entity.HasComponent<ScriptComponent>())
			ScriptEngine::OnScriptComponentDestroyed(m_SceneID, entity.GetUUID());
		if (entity.HasComponent<Audio::AudioComponent>())
			Audio::MiniAudioEngine::Get().UnregisterAudioComponent(m_SceneID, entity.GetUUID());

		m_Registry.destroy(entity.m_EntityHandle);
	}

	template<typename T>
	static void CopyComponent(entt::registry& dstRegistry, entt::registry& srcRegistry, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto components = srcRegistry.view<T>();
		for (auto srcEntity : components)
		{
			entt::entity destEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

			auto& srcComponent = srcRegistry.get<T>(srcEntity);
			auto& destComponent = dstRegistry.emplace_or_replace<T>(destEntity, srcComponent);
		}
	}

	template<typename T>
	static void CopyComponentIfExists(entt::entity dst, entt::entity src, entt::registry& registry)
	{
		if (registry.has<T>(src))
		{
			auto& srcComponent = registry.get<T>(src);
			registry.emplace_or_replace<T>(dst, srcComponent);
		}
	}

	void Scene::DuplicateEntity(Entity entity)
	{
		Entity newEntity;
		if (entity.HasComponent<TagComponent>())
			newEntity = CreateEntity(entity.GetComponent<TagComponent>().Tag);
		else
			newEntity = CreateEntity();

		CopyComponentIfExists<TransformComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<RelationshipComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MeshComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<DirectionalLightComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SkyLightComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<ScriptComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CameraComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<RigidBodyComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<BoxColliderComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<SphereColliderComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<CapsuleColliderComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<MeshColliderComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<Audio::AudioComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
		CopyComponentIfExists<AudioListenerComponent>(newEntity.m_EntityHandle, entity.m_EntityHandle, m_Registry);
	}

	Entity Scene::FindEntityByTag(const std::string& tag)
	{
		// TODO: If this becomes used often, consider indexing by tag
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const auto& canditate = view.get<TagComponent>(entity).Tag;
			if (canditate == tag)
				return Entity(entity, this);
		}

		return Entity{};
	}

	Entity Scene::FindEntityByUUID(UUID id)
	{
		auto view = m_Registry.view<IDComponent>();
		for (auto entity : view)
		{
			auto& idComponent = m_Registry.get<IDComponent>(entity);
			if (idComponent.ID == id)
				return Entity(entity, this);
		}

		return Entity{};
	}

	void Scene::ConvertToLocalSpace(Entity entity)
	{
		Entity parent = FindEntityByUUID(entity.GetParentUUID());

		if (!parent)
			return;

		auto& transform = entity.Transform();
		glm::mat4 parentTransform = GetWorldSpaceTransformMatrix(parent);

		glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();
		Math::DecomposeTransform(localTransform, transform.Translation, transform.Rotation, transform.Scale);
	}

	void Scene::ConvertToWorldSpace(Entity entity)
	{
		Entity parent = FindEntityByUUID(entity.GetParentUUID());

		if (!parent)
			return;

		glm::mat4 transform = GetTransformRelativeToParent(entity);
		auto& entityTransform = entity.Transform();
		Math::DecomposeTransform(transform, entityTransform.Translation, entityTransform.Rotation, entityTransform.Scale);
	}

	glm::mat4 Scene::GetTransformRelativeToParent(Entity entity)
	{
		glm::mat4 transform(1.0f);

		Entity parent = FindEntityByUUID(entity.GetParentUUID());
		if (parent)
			transform = GetTransformRelativeToParent(parent);

		return transform * entity.Transform().GetTransform();
	}

	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
	{
		glm::mat4 transform = entity.Transform().GetTransform();

		while (Entity parent = FindEntityByUUID(entity.GetParentUUID()))
		{
			transform = parent.Transform().GetTransform() * transform;
			entity = parent;
		}

		return transform;
	}

	// TODO: Definitely cache this at some point
	TransformComponent Scene::GetWorldSpaceTransform(Entity entity)
	{
		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		TransformComponent transformComponent;

		Math::DecomposeTransform(transform, transformComponent.Translation, transformComponent.Rotation, transformComponent.Scale);

		glm::quat rotationQuat = glm::quat(transformComponent.Rotation);
		transformComponent.Up = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 1.0f, 0.0f)));
		transformComponent.Right = glm::normalize(glm::rotate(rotationQuat, glm::vec3(1.0f, 0.0f, 0.0f)));
		transformComponent.Forward = glm::normalize(glm::rotate(rotationQuat, glm::vec3(0.0f, 0.0f, -1.0f)));

		return transformComponent;
	}

	void Scene::ParentEntity(Entity entity, Entity parent)
	{
		if (parent.IsDescendantOf(entity))
		{
			UnparentEntity(parent);

			Entity newParent = FindEntityByUUID(entity.GetParentUUID());
			if (newParent)
			{
				UnparentEntity(entity);
				ParentEntity(parent, newParent);
			}
		}
		else
		{
			Entity previousParent = FindEntityByUUID(entity.GetParentUUID());

			if (previousParent)
				UnparentEntity(entity);
		}

		entity.SetParentUUID(parent.GetUUID());
		parent.Children().push_back(entity.GetUUID());

		ConvertToLocalSpace(entity);
	}

	void Scene::UnparentEntity(Entity entity)
	{
		Entity parent = FindEntityByUUID(entity.GetParentUUID());

		if (!parent)
			return;

		auto& parentChildren = parent.Children();
		parentChildren.erase(std::remove(parentChildren.begin(), parentChildren.end(), entity.GetUUID()), parentChildren.end());

		ConvertToWorldSpace(entity);

		entity.SetParentUUID(0);
	}

	// Copy to runtime
	void Scene::CopyTo(Ref<Scene>& target)
	{
		// Environment
		target->m_Light = m_Light;
		target->m_LightMultiplier = m_LightMultiplier;

		target->m_Environment = m_Environment;
		target->m_SkyboxTexture = m_SkyboxTexture;
		target->m_SkyboxMaterial = m_SkyboxMaterial;
		target->m_SkyboxLod = m_SkyboxLod;

		std::unordered_map<UUID, entt::entity> enttMap;
		auto idComponents = m_Registry.view<IDComponent>();
		for (auto entity : idComponents)
		{
			auto uuid = m_Registry.get<IDComponent>(entity).ID;
			Entity e = target->CreateEntityWithID(uuid, "", true);
			enttMap[uuid] = e.m_EntityHandle;
		}

		CopyComponent<TagComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<TransformComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RelationshipComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MeshComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<DirectionalLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SkyLightComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<ScriptComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CameraComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SpriteRendererComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RigidBody2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<BoxCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CircleCollider2DComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<RigidBodyComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<BoxColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<SphereColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<CapsuleColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<MeshColliderComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<Audio::AudioComponent>(target->m_Registry, m_Registry, enttMap);
		CopyComponent<AudioListenerComponent>(target->m_Registry, m_Registry, enttMap);

		const auto& entityInstanceMap = ScriptEngine::GetEntityInstanceMap();
		if (entityInstanceMap.find(target->GetUUID()) != entityInstanceMap.end())
			ScriptEngine::CopyEntityScriptData(target->GetUUID(), m_SceneID);

		target->SetPhysics2DGravity(GetPhysics2DGravity());
	}

	Ref<Scene> Scene::GetScene(UUID uuid)
	{
		if (s_ActiveScenes.find(uuid) != s_ActiveScenes.end())
			return s_ActiveScenes.at(uuid);

		return {};
	}

	float Scene::GetPhysics2DGravity() const
	{
		return m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->GetGravity().y;
	}

	void Scene::SetPhysics2DGravity(float gravity)
	{
		m_Registry.get<Box2DWorldComponent>(m_SceneEntity).World->SetGravity({ 0.0f, gravity });
	}
}
