#pragma once

#include "Haoyue/Scene/Scene.h"
#include "Haoyue/Scene/Components.h"
#include "Haoyue/Renderer/Mesh.h"
#include "RenderPass.h"

#include "Haoyue/Renderer/UniformBufferSet.h"
#include "Haoyue/Renderer/RenderCommandBuffer.h"

#include <map>

namespace Haoyue {

	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowBoundingBoxes = false;
	};

	struct SceneRendererCamera
	{
		Haoyue::Camera Camera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	class SceneRenderer : public RefCounted
	{
	public:
		SceneRenderer(Ref<Scene> scene);
		~SceneRenderer();

		void Init();

		void SetScene(Ref<Scene> scene);

		void SetViewportSize(uint32_t width, uint32_t height);

		void BeginScene(const SceneRendererCamera& camera);
		void EndScene();

		void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = nullptr);
		void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));
		void SubmitColliderMesh(const BoxColliderComponent& component, const glm::mat4& parentTransform = glm::mat4(1.0F));
		void SubmitColliderMesh(const SphereColliderComponent& component, const glm::mat4& parentTransform = glm::mat4(1.0F));
		void SubmitColliderMesh(const CapsuleColliderComponent& component, const glm::mat4& parentTransform = glm::mat4(1.0F));
		void SubmitColliderMesh(const MeshColliderComponent& component, const glm::mat4& parentTransform = glm::mat4(1.0F));

		Ref<RenderPass> GetFinalRenderPass();
		Ref<Image2D> GetFinalPassImage();

		SceneRendererOptions& GetOptions();

		void SetShadowSettings(float nearPlane, float farPlane, float lambda)
		{
			CascadeNearPlaneOffset = nearPlane;
			CascadeFarPlaneOffset = farPlane;
			CascadeSplitLambda = lambda;
		}

		void OnImGuiRender();
		static void WaitForThreads();
	private:
		void FlushDrawList();
		void ShadowMapPass();
		void GeometryPass();
		void CompositePass();
		void BloomBlurPass();

		struct CascadeData
		{
			glm::mat4 ViewProj;
			glm::mat4 View;
			float SplitDepth;
		};
		void CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection);
	private:
		Ref<Scene> m_Scene;
		Ref<RenderCommandBuffer> m_CommandBuffer;

		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			// Resources
			Ref<Environment> SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
			LightEnvironment SceneLightEnvironment;
			Light ActiveLight;
		} m_SceneData;

		Ref<Shader> m_CompositeShader;
		Ref<Shader> m_BloomBlurShader;
		Ref<Shader> m_BloomBlendShader;

		struct UBCamera
		{
			glm::mat4 ViewProjection;
			glm::mat4 InverseViewProjection;
			glm::mat4 View;
		} CameraData;

		struct UBShadow
		{
			glm::mat4 ViewProjection[4];
		} ShadowData;

		struct Light
		{
			glm::vec3 Direction;
			float Padding = 0.0f;
			glm::vec3 Radiance;
			float Multiplier;
		};

		struct UBScene
		{
			Light lights;
			glm::vec3 u_CameraPosition;
		} SceneDataUB;

		struct UBRendererData
		{
			glm::vec4 u_CascadeSplits;
			bool ShowCascades = false;
			char Padding0[3] = { 0,0,0 }; // Bools are 4-bytes in GLSL
			bool SoftShadows = true;
			char Padding1[3] = { 0,0,0 };
			float LightSize = 0.5f;
			float MaxShadowDistance = 200.0f;
			float ShadowFade = 1.0f;
			bool CascadeFading = true;
			char Padding2[3] = { 0,0,0 };
			float CascadeTransitionFade = 1.0f;
		} RendererDataUB;

		Ref<UniformBufferSet> m_UniformBufferSet;

		Ref<Shader> ShadowMapShader, ShadowMapAnimShader;
		Ref<RenderPass> ShadowMapRenderPass[4];
		float LightDistance = 0.1f;
		float CascadeSplitLambda = 0.98f;
		glm::vec4 CascadeSplits;
		float CascadeFarPlaneOffset = 15.0f, CascadeNearPlaneOffset = -15.0f;

		bool EnableBloom = false;
		float BloomThreshold = 1.5f;

		glm::vec2 FocusPoint = { 0.5f, 0.5f };

		RendererID ShadowMapSampler;
		Ref<Material> CompositeMaterial;

		Ref<Pipeline> m_GeometryPipeline;
		Ref<Pipeline> m_CompositePipeline;
		Ref<Pipeline> m_ShadowPassPipeline;
		Ref<Material> m_ShadowPassMaterial;
		Ref<Pipeline> m_SkyboxPipeline;
		Ref<Material> m_SkyboxMaterial;

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<Material> Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> m_DrawList;
		std::vector<DrawCommand> m_SelectedMeshDrawList;
		std::vector<DrawCommand> m_ColliderDrawList;
		std::vector<DrawCommand> m_ShadowPassDrawList;

		// Grid
		Ref<Pipeline> m_GridPipeline;
		Ref<Shader> m_GridShader;
		Ref<Material> m_GridMaterial;
		Ref<Material> m_OutlineMaterial, OutlineAnimMaterial;
		Ref<Material> m_ColliderMaterial;

		SceneRendererOptions m_Options;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_NeedsResize = false;
		bool m_Active = false;
		bool m_ResourcesCreated = false;
	};

}
