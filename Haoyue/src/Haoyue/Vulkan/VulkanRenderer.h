#pragma once

#include "Haoyue/Renderer/RendererAPI.h"

#include "vulkan/vulkan.h"

namespace Haoyue {

	class VulkanRenderer : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual RendererCapabilities& GetCapabilities() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass, bool explicitClear = false) override;
		virtual void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer) override;
		virtual void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material) override;

		virtual void SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow) override;
		virtual std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath) override;
		virtual Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) override;

		virtual void RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms = Buffer()) override;
		virtual void RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, const glm::mat4& transform) override;
		virtual void RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount = 0) override;
	public:
		static VkDescriptorSet RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);
	};

}