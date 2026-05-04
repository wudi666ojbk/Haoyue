#include "pch.h"
#include "SceneRenderer.h"

#include "Renderer.h"
#include "SceneEnvironment.h"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer2D.h"
#include "UniformBuffer.h"

#include "Haoyue/ImGui/ImGui.h"

namespace Haoyue {

	static std::vector<std::thread> s_ThreadPool;

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_Scene(scene)
	{
		Init();
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	void SceneRenderer::Init()
	{
		m_CommandBuffer = RenderCommandBuffer::Create(0, "SceneRenderer");

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		m_UniformBufferSet = UniformBufferSet::Create(framesInFlight);
		m_UniformBufferSet->Create(sizeof(UBCamera), 0);
		m_UniformBufferSet->Create(sizeof(UBShadow), 1);
		m_UniformBufferSet->Create(sizeof(UBScene), 2);
		m_UniformBufferSet->Create(sizeof(UBRendererData), 3);

		m_CompositeShader = Renderer::GetShaderLibrary()->Get("SceneComposite");
		CompositeMaterial = Material::Create(m_CompositeShader);

		// Shadow pass
		{
			ImageSpecification spec;
			spec.Format = ImageFormat::DEPTH32F;
			spec.Usage = ImageUsage::Attachment;
			spec.Width = 4096;
			spec.Height = 4096;
			spec.Layers = 4; // 4 cascades
			Ref<Image2D> cascadedDepthImage = Image2D::Create(spec);
			cascadedDepthImage->Invalidate();
			cascadedDepthImage->CreatePerLayerImageViews();

			FramebufferSpecification shadowMapFramebufferSpec;
			shadowMapFramebufferSpec.Width = 4096;
			shadowMapFramebufferSpec.Height = 4096;
			shadowMapFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
			shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			shadowMapFramebufferSpec.NoResize = true;
			shadowMapFramebufferSpec.ExistingImage = cascadedDepthImage;

			// 4 cascades
			for (int i = 0; i < 4; i++)
			{
				shadowMapFramebufferSpec.ExistingImageLayer = i;

				RenderPassSpecification shadowMapRenderPassSpec;
				shadowMapRenderPassSpec.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);
				shadowMapRenderPassSpec.DebugName = "ShadowMap";
				ShadowMapRenderPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
			}

			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("ShadowMap");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "ShadowPass";
			pipelineSpec.Shader = shadowPassShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = ShadowMapRenderPass[0];
			m_ShadowPassPipeline = Pipeline::Create(pipelineSpec);
			m_ShadowPassMaterial = Material::Create(shadowPassShader, "ShadowPass");
		}

		// Geometry
		{
			FramebufferSpecification geoFramebufferSpec;
			geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA32F, ImageFormat::Depth };
			geoFramebufferSpec.Samples = 1;
			geoFramebufferSpec.ClearColor = { 0.1f, 0.5f, 0.1f, 1.0f };

			Ref<Framebuffer> framebuffer = Framebuffer::Create(geoFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("PBR_Static");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "Geometry";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "PBR-Static";
			m_GeometryPipeline = Pipeline::Create(pipelineSpecification);

			pipelineSpecification.Wireframe = true;
			pipelineSpecification.DepthTest = false;
			pipelineSpecification.LineWidth = 2.0f;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Wireframe");
			pipelineSpecification.DebugName = "Wireframe";
			m_GeometryWireframePipeline = Pipeline::Create(pipelineSpecification);
		}

		// Composite
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.Attachments = { ImageFormat::RGBA, ImageFormat::Depth };
			compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

			Ref<Framebuffer> framebuffer = Framebuffer::Create(compFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SceneComposite");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "Composite";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "SceneComposite";
			pipelineSpecification.DepthWrite = false;
			m_CompositePipeline = Pipeline::Create(pipelineSpecification);
		}

		// External compositing
		{
			FramebufferSpecification extCompFramebufferSpec;
			extCompFramebufferSpec.Attachments = { ImageFormat::RGBA, ImageFormat::Depth };
			extCompFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			extCompFramebufferSpec.ClearOnLoad = false;
			extCompFramebufferSpec.ExistingFramebuffer = m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;

			Ref<Framebuffer> framebuffer = Framebuffer::Create(extCompFramebufferSpec);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "External-Composite";
			m_ExternalCompositeRenderPass = RenderPass::Create(renderPassSpec);
		}

		// Grid
		{
			m_GridShader = Renderer::GetShaderLibrary()->Get("Grid");
			const float gridScale = 4.0f;    // 减小scale让网格变大（更稀疏）
			const float gridSize = 0.001f;   // 进一步减小size让线变得更细
			m_GridMaterial = Material::Create(m_GridShader);
			m_GridMaterial->Set("u_Settings.Scale", gridScale);
			m_GridMaterial->Set("u_Settings.Size", gridSize);

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = m_GridShader;
			pipelineSpec.BackfaceCulling = false;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = m_GeometryPipeline->GetSpecification().RenderPass;
			m_GridPipeline = Pipeline::Create(pipelineSpec);
		}

		// Collider
		//auto colliderShader = Shader::Create("assets/shaders/Collider.glsl");
		//ColliderMaterial = Material::Create(Material::Create(colliderShader));
		//ColliderMaterial->SetFlag(MaterialFlag::DepthTest, false);
		m_WireframeMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Wireframe"));
		m_WireframeMaterial->Set("u_MaterialUniforms.Color", { 1.0f, 0.5f, 0.0f, 1.0f });
		m_ColliderMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Wireframe"));
		m_ColliderMaterial->Set("u_MaterialUniforms.Color", { 0.2f, 1.0f, 0.2f, 1.0f });

		// Skybox
		{
			auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Skybox";
			pipelineSpec.Shader = skyboxShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = m_GeometryPipeline->GetSpecification().RenderPass;
			m_SkyboxPipeline = Pipeline::Create(pipelineSpec);

			m_SkyboxMaterial = Material::Create(skyboxShader);
			m_SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, false);
		}

		Ref<SceneRenderer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->m_ResourcesCreated = true;
			});
	}

	void SceneRenderer::SetScene(Ref<Scene> scene)
	{
		HY_CORE_ASSERT(!m_Active, "Can't change scenes while rendering");
		m_Scene = scene;
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (m_ViewportWidth != width || m_ViewportHeight != height)
		{
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			m_NeedsResize = true;
		}
	}

	void SceneRenderer::CalculateCascades(SceneRenderer::CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection)
	{
		struct FrustumBounds
		{
			float r, l, b, t, f, n;
		};

		FrustumBounds frustumBounds[3];

		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		// TODO: less hard-coding!
		float nearClip = 0.1f;
		float farClip = 1000.0f;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		// Manually set cascades here
		// cascadeSplits[0] = 0.05f;
		// cascadeSplits[1] = 0.15f;
		// cascadeSplits[2] = 0.3f;
		// cascadeSplits[3] = 1.0f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			const float ShadowMapResolution = 4096.0f;
			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		HY_CORE_ASSERT(m_Scene);
		HY_CORE_ASSERT(!m_Active);
		m_Active = true;

		if (!m_ResourcesCreated)
			return;

		m_SceneData.SceneCamera = camera;
		m_SceneData.SceneEnvironment = m_Scene->m_Environment;
		m_SceneData.SceneEnvironmentIntensity = m_Scene->m_EnvironmentIntensity;
		m_SceneData.ActiveLight = m_Scene->m_Light;
		m_SceneData.SceneLightEnvironment = m_Scene->m_LightEnvironment;
		m_SceneData.SkyboxLod = m_Scene->m_SkyboxLod;
		m_SceneData.ActiveLight = m_Scene->m_Light;

		if (m_NeedsResize)
		{
			m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_ExternalCompositeRenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_NeedsResize = false;
		}

		// Update uniform buffers
		UBCamera& cameraData = CameraData;
		UBScene& sceneData = SceneDataUB;
		UBShadow& shadowData = ShadowData;
		UBRendererData& rendererData = RendererDataUB;

		auto& sceneCamera = m_SceneData.SceneCamera;
		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(sceneCamera.ViewMatrix)[3];

		auto inverseVP = glm::inverse(viewProjection);
		cameraData.ViewProjection = viewProjection;
		cameraData.InverseViewProjection = inverseVP;
		cameraData.View = sceneCamera.ViewMatrix;
		Ref<SceneRenderer> instance = this;
		Renderer::Submit([instance, cameraData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->m_UniformBufferSet->Get(0, 0, bufferIndex)->RT_SetData(&cameraData, sizeof(cameraData));
			});

		const auto& directionalLight = m_SceneData.SceneLightEnvironment.DirectionalLights[0];
		sceneData.lights.Direction = directionalLight.Direction;
		sceneData.lights.Radiance = directionalLight.Radiance;
		sceneData.lights.Multiplier = directionalLight.Multiplier;
		sceneData.u_CameraPosition = cameraPosition;
		Renderer::Submit([instance, sceneData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->m_UniformBufferSet->Get(2, 0, bufferIndex)->RT_SetData(&sceneData, sizeof(sceneData));
			});

		CascadeData cascades[4];
		CalculateCascades(cascades, sceneCamera, directionalLight.Direction);

		// TODO: four cascades for now
		for (int i = 0; i < 4; i++)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			shadowData.ViewProjection[i] = cascades[i].ViewProj;
		}
		Renderer::Submit([instance, shadowData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->m_UniformBufferSet->Get(1, 0, bufferIndex)->RT_SetData(&shadowData, sizeof(shadowData));
			});

		rendererData.u_CascadeSplits = CascadeSplits;
		Renderer::Submit([instance, rendererData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->m_UniformBufferSet->Get(3, 0, bufferIndex)->RT_SetData(&rendererData, sizeof(rendererData));
			});

		Renderer::SetSceneEnvironment(this, m_SceneData.SceneEnvironment, m_ShadowPassPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage());
	}

	void SceneRenderer::EndScene()
	{
		HY_CORE_ASSERT(m_Active);
#if MULTI_THREAD
		Ref<SceneRenderer> instance = this;
		s_ThreadPool.emplace_back(([instance]() mutable
			{
				instance->FlushDrawList();
			}));
#else 
		FlushDrawList();
#endif

		m_Active = false;
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		m_DrawList.push_back({ mesh, overrideMaterial, transform });
		m_ShadowPassDrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		m_SelectedMeshDrawList.push_back({ mesh, nullptr, transform });
		m_ShadowPassDrawList.push_back({ mesh, nullptr, transform });
	}

	void SceneRenderer::SubmitColliderMesh(const BoxColliderComponent& component, const glm::mat4& parentTransform)
	{
		m_ColliderDrawList.push_back({ component.DebugMesh, nullptr, glm::translate(parentTransform, component.Offset) });
	}

	void SceneRenderer::SubmitColliderMesh(const SphereColliderComponent& component, const glm::mat4& parentTransform)
	{
		m_ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const CapsuleColliderComponent& component, const glm::mat4& parentTransform)
	{
		m_ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const MeshColliderComponent& component, const glm::mat4& parentTransform)
	{
		for (auto debugMesh : component.ProcessedMeshes)
			m_ColliderDrawList.push_back({ debugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::ShadowMapPass()
	{
		auto& directionalLights = m_SceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
		{
			for (int i = 0; i < 4; i++)
			{
				// Clear shadow maps
				Renderer::BeginRenderPass(m_CommandBuffer, ShadowMapRenderPass[i]);
				Renderer::EndRenderPass(m_CommandBuffer);
			}
			return;
		}

		// TODO: change to four cascades (or set number)
		for (int i = 0; i < 4; i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, ShadowMapRenderPass[i]);

			// static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });

			// Render entities
			Buffer cascade(&i, sizeof(uint32_t));
			for (auto& dc : m_ShadowPassDrawList)
			{
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipeline, m_UniformBufferSet, dc.Mesh, dc.Transform, m_ShadowPassMaterial, cascade);
			}

			Renderer::EndRenderPass(m_CommandBuffer);
		}
	}

	void SceneRenderer::GeometryPass()
	{
		Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryPipeline->GetSpecification().RenderPass);
		// Skybox
		m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SceneData.SkyboxLod);

		Ref<TextureCube> radianceMap = m_SceneData.SceneEnvironment ? m_SceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		m_SkyboxMaterial->Set("u_Texture", radianceMap);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_SkyboxPipeline, m_UniformBufferSet, m_SkyboxMaterial);

		// Render entities
		for (auto& dc : m_DrawList)
			Renderer::RenderMesh(m_CommandBuffer, m_GeometryPipeline, m_UniformBufferSet, dc.Mesh, dc.Transform);

		for (auto& dc : m_SelectedMeshDrawList)
		{
			Renderer::RenderMesh(m_CommandBuffer, m_GeometryPipeline, m_UniformBufferSet, dc.Mesh, dc.Transform);
			if (m_Options.ShowSelectedInWireframe)
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_GeometryWireframePipeline, m_UniformBufferSet, dc.Mesh, dc.Transform, m_WireframeMaterial);
		}

		if (m_Options.ShowCollidersWireframe)
		{
			for (DrawCommand& dc : m_ColliderDrawList)
			{
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_GeometryWireframePipeline, m_UniformBufferSet, dc.Mesh, dc.Transform, m_ColliderMaterial);
			}
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			// Extract camera position from view matrix
			glm::vec3 cameraPos = glm::inverse(m_SceneData.SceneCamera.ViewMatrix)[3];
			
			// Create a very large grid that follows the camera on XZ plane
			const glm::mat4 transform = 
				glm::translate(glm::mat4(1.0f), glm::vec3(cameraPos.x, 0.0f, cameraPos.z)) *
				glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * 
				glm::scale(glm::mat4(1.0f), glm::vec3(500.0f));
			Renderer::RenderQuad(m_CommandBuffer, m_GridPipeline, m_UniformBufferSet, m_GridMaterial, transform);
		}

		if (GetOptions().ShowBoundingBoxes)
		{
#if 0
			Renderer2D::BeginScene(viewProjection);
			for (auto& dc : DrawList)
				Renderer::DrawAABB(dc.Mesh, dc.Transform);
			Renderer2D::EndScene();
#endif
		}

		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginRenderPass(m_CommandBuffer, m_CompositePipeline->GetSpecification().RenderPass);

		auto framebuffer = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
		// float exposure = SceneData.SceneCamera.Camera.GetExposure();
		float exposure = m_SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = framebuffer->GetSpecification().Samples;

		CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		//CompositeMaterial->Set("u_Uniforms.TextureSamples", textureSamples);

		CompositeMaterial->Set("u_Texture", framebuffer->GetImage());

		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_CompositePipeline, nullptr, CompositeMaterial);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRenderer::BloomBlurPass()
	{
	}

	void SceneRenderer::FlushDrawList()
	{
		if (m_ResourcesCreated)
		{
			m_CommandBuffer->Begin();
			ShadowMapPass();
			GeometryPass();
			CompositePass();
			m_CommandBuffer->End();
			m_CommandBuffer->Submit();
			//	BloomBlurPass();
		}

		m_DrawList.clear();
		m_SelectedMeshDrawList.clear();
		m_ShadowPassDrawList.clear();
		m_ColliderDrawList.clear();
		m_SceneData = {};
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return m_CompositePipeline->GetSpecification().RenderPass;
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage()
	{
		if (!m_ResourcesCreated)
			return nullptr;

		return m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage();
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return m_Options;
	}

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin("Scene Renderer");

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Shader::s_AllShaders;
			for (auto& shader : shaders)
			{
				if (ImGui::TreeNode(shader->GetName().c_str()))
				{
					std::string buttonName = "Reload##" + shader->GetName();
					if (ImGui::Button(buttonName.c_str()))
						shader->Reload(true);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		if (UI::BeginTreeNode("Shadows"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Soft Shadows", RendererDataUB.SoftShadows);
			UI::Property("Light Size", RendererDataUB.LightSize, 0.01f);
			UI::Property("Max Shadow Distance", RendererDataUB.MaxShadowDistance, 1.0f);
			UI::Property("Shadow Fade", RendererDataUB.ShadowFade, 5.0f);
			UI::EndPropertyGrid();
			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid();
				UI::Property("Show Cascades", RendererDataUB.ShowCascades);
				UI::Property("Cascade Fading", RendererDataUB.CascadeFading);
				UI::Property("Cascade Transition Fade", RendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
				UI::Property("Cascade Split", CascadeSplitLambda, 0.01f);
				UI::Property("CascadeNearPlaneOffset", CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
				UI::Property("CascadeFarPlaneOffset", CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			if (UI::BeginTreeNode("Shadow Map", false))
			{
				static int cascadeIndex = 0;
				auto fb = ShadowMapRenderPass[cascadeIndex]->GetSpecification().TargetFramebuffer;
				auto image = fb->GetDepthImage();

				float size = ImGui::GetContentRegionAvailWidth(); // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
				UI::BeginPropertyGrid();
				UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
				UI::EndPropertyGrid();
				UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}
		ImGui::End();
	}

	void SceneRenderer::WaitForThreads()
	{
		for (uint32_t i = 0; i < s_ThreadPool.size(); i++)
			s_ThreadPool[i].join();

		s_ThreadPool.clear();
	}

}
