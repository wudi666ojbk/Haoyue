#include "pch.h"
#include "Renderer.h"

#include "Shader.h"

#include <map>

#include "RendererAPI.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"

#include "Haoyue/Core/Timer.h"

#include "Haoyue/Vulkan/VulkanRenderer.h"
#include "Haoyue/Vulkan/VulkanContext.h"

namespace Haoyue {

	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<Material>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline)
	{
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material)
	{
		s_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void Renderer::OnShaderReloaded(size_t hash)
	{
		if (s_ShaderDependencies.find(hash) != s_ShaderDependencies.end())
		{
			auto& dependencies = s_ShaderDependencies.at(hash);
			for (auto& pipeline : dependencies.Pipelines)
			{
				pipeline->Invalidate();
			}

			for (auto& material : dependencies.Materials)
			{
				material->Invalidate();
			}
		}
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return Application::Get().GetWindow().GetSwapChain().GetCurrentBufferIndex();
	}

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		// TODO: make sure this is called at a valid time
		s_CurrentRendererAPI = api;
	}

	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> WhiteTexture;
		Ref<TextureCube> BlackCubeTexture;
		Ref<Environment> EmptyEnvironment;
	};

	static RendererData* s_Data = nullptr;
	static RenderCommandQueue* s_CommandQueue = nullptr;
	static RenderCommandQueue s_ResourceFreeQueue[3];

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::Vulkan: return new VulkanRenderer();
		}
		HY_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void Renderer::Init()
	{
		s_Data = new RendererData();
		s_CommandQueue = new RenderCommandQueue();

		//Renderer::GetConfig().FramesInFlight = 1;

		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// Compute shaders
		Renderer::GetShaderLibrary()->Load("Resources/shaders/EnvironmentMipFilter.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/shaders/EquirectangularToCubeMap.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/shaders/EnvironmentIrradiance.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/shaders/PreethamSky.glsl");

		Renderer::GetShaderLibrary()->Load("Resources/shaders/Grid.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/shaders/SceneComposite.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/shaders/PBR_Static.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/shaders/Skybox.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/shaders/ShadowMap.glsl");

		// Compile shaders
		Renderer::WaitAndRender();

		uint32_t whiteTextureData = 0xffffffff;
		s_Data->WhiteTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &whiteTextureData);

		uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = TextureCube::Create(ImageFormat::RGBA, 1, 1, &blackTextureData);

		s_Data->EmptyEnvironment = Ref<Environment>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		s_RendererAPI->Init();
	}

	void Renderer::Shutdown()
	{
		s_ShaderDependencies.clear();
		s_RendererAPI->Shutdown();

		delete s_Data;
		delete s_CommandQueue;
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	void Renderer::WaitAndRender()
	{
		HY_SCOPE_PERF("Renderer::WaitAndRender");
		s_CommandQueue->Execute();
	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool clear)
	{
		HY_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		s_RendererAPI->BeginRenderPass(renderCommandBuffer, renderPass);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndRenderPass(renderCommandBuffer);
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow)
	{
		s_RendererAPI->SetSceneEnvironment(sceneRenderer, environment, shadow);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	Ref<TextureCube> Renderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		return s_RendererAPI->CreatePreethamSky(turbidity, azimuth, inclination);
	}

	void Renderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMesh(renderCommandBuffer, pipeline, uniformBufferSet, mesh, transform);
	}

	void Renderer::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> material, Buffer additionalUniforms)
	{
		s_RendererAPI->RenderMeshWithMaterial(renderCommandBuffer, pipeline, uniformBufferSet, mesh, material, transform, additionalUniforms);
	}

	void Renderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(renderCommandBuffer, pipeline, uniformBufferSet, material, transform);
	}

	void Renderer::SubmitQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, const glm::mat4& transform)
	{
		/*bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);

			auto shader = material->GetShader();
			shader->SetUniformBuffer("Transform", &transform, sizeof(glm::mat4));
		}

		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		s_Data->m_FullscreenQuadPipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);*/
	}

	void Renderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
	{
		s_RendererAPI->SubmitFullscreenQuad(renderCommandBuffer, pipeline, uniformBufferSet, material);
	}

	void Renderer::DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		const auto& meshAssetSubmeshes = mesh->GetMeshAsset()->GetSubmeshes();
		auto& submeshes = mesh->GetSubmeshes();
		for (uint32_t submeshIndex : submeshes)
		{
			const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
			auto& aabb = submesh.BoundingBox;
			auto aabbTransform = transform * submesh.Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void Renderer::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
	{
		glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
		glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

		glm::vec4 corners[8] =
		{
			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },

			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
		};

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[(i + 1) % 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[i + 4], color);
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}


	Ref<Environment> Renderer::GetEmptyEnvironment()
	{
		return s_Data->EmptyEnvironment;
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue;
	}

	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}

	RendererConfig& Renderer::GetConfig()
	{
		static RendererConfig config;
		return config;
	}

}
