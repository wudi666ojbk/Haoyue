#pragma once

#include "RendererContext.h"
#include "RenderCommandQueue.h"
#include "RenderPass.h"
#include "Mesh.h"

#include "Haoyue/Core/Application.h"

#include "RendererCapabilities.h"

#include "Haoyue/Scene/Scene.h"

namespace Haoyue {

	class ShaderLibrary;

	struct RendererConfig
	{
		uint32_t FramesInFlight = 3;
		// "Experimental" features
		bool ComputeEnvironmentMaps = true;

		// Tiering settings
		uint32_t EnvironmentMapResolution = 1024;
		uint32_t IrradianceMapComputeSamples = 512;
	};

	class Renderer
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		static Ref<RendererContext> GetContext()
		{
			return Application::Get().GetWindow().GetRenderContext();
		}

		static void Init();
		static void Shutdown();

		static RendererCapabilities& GetCapabilities();

		static Ref<ShaderLibrary> GetShaderLibrary();

		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
				};
			auto storageBuffer = GetRenderCommandQueue().Allocate(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}

		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
				};

			Submit([renderCmd, func]()
			{
				uint32_t index = Renderer::GetCurrentFrameIndex();
				auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
				new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
			});
		}

		static void WaitAndRender();

		// ~Actual~ Renderer here... TODO: remove confusion later
		static void BeginRenderPass(Ref<RenderPass> renderPass, bool clear = true);
		static void EndRenderPass();

		static void BeginFrame();
		static void EndFrame();

		static void SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow);
		static std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath);
		static Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination);

		static void RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform);
		static void RenderMeshWithMaterial(Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer());
		static void RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform);
		static void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material);

		static void SubmitQuad(Ref<Material> material, const glm::mat4& transform = glm::mat4(1.0f));
		static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial = nullptr);

		static void DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

		static Ref<Texture2D> GetWhiteTexture();
		static Ref<TextureCube> GetBlackCubeTexture();
		static Ref<Environment> GetEmptyEnvironment();

		static void SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t frame, uint32_t set);
		static Ref<UniformBuffer> GetUniformBuffer(uint32_t frame, uint32_t binding, uint32_t set = 0);

		static void RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline);
		static void RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material);
		static void OnShaderReloaded(size_t hash);

		static uint32_t GetCurrentFrameIndex();

		static RendererConfig& GetConfig();

		static RenderCommandQueue& GetRenderResourceReleaseQueue(uint32_t index);
	private:
		static RenderCommandQueue& GetRenderCommandQueue();
	};

	namespace Utils {

		inline void DumpGPUInfo()
		{
			auto& caps = Renderer::GetCapabilities();
			HY_CORE_TRACE("GPU Info:");
			HY_CORE_TRACE("  Vendor: {0}", caps.Vendor);
			HY_CORE_TRACE("  Device: {0}", caps.Device);
			HY_CORE_TRACE("  Version: {0}", caps.Version);
		}

	}

}
