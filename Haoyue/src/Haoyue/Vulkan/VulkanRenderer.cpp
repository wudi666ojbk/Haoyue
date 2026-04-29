#include "pch.h"
#include "VulkanRenderer.h"

#include "imgui.h"

#include "Vulkan.h"
#include "VulkanContext.h"

#include "Haoyue/Renderer/Renderer.h"
#include "Haoyue/Renderer/SceneRenderer.h"

#include "VulkanPipeline.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanFramebuffer.h"
#include "VulkanMaterial.h"
#include "VulkanUniformBuffer.h"
#include "VulkanRenderCommandBuffer.h"

#include "VulkanShader.h"
#include "VulkanTexture.h"

#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "VulkanComputePipeline.h"
#include "Haoyue/Core/Timer.h"

#include <glm/glm.hpp>

namespace Haoyue {

	struct VulkanRendererData
	{
		RendererCapabilities RenderCaps;

		Ref<Texture2D> BRDFLut;

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		VulkanShader::ShaderMaterialDescriptorSet QuadDescriptorSet;

		std::unordered_map<SceneRenderer*, std::vector<VulkanShader::ShaderMaterialDescriptorSet>> RendererDescriptorSet;
		VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
		std::vector<VkDescriptorPool> DescriptorPools;
		std::vector<uint32_t> DescriptorPoolAllocationCount;

		// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
		std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
	};

	static VulkanRendererData* s_Data = nullptr;

	namespace Utils {

		static const char* VulkanVendorIDToString(uint32_t vendorID)
		{
			switch (vendorID)
			{
				case 0x10DE: return "NVIDIA";
				case 0x1002: return "AMD";
				case 0x8086: return "INTEL";
				case 0x13B5: return "ARM";
			}
			return "Unknown";
		}

	}

	void VulkanRenderer::Init()
	{
		s_Data = new VulkanRendererData();
		const auto& config = Renderer::GetConfig();
		s_Data->DescriptorPools.resize(config.FramesInFlight);
		s_Data->DescriptorPoolAllocationCount.resize(config.FramesInFlight);

		auto& caps = s_Data->RenderCaps;
		auto& properties = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
		caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
		caps.Device = properties.deviceName;
		caps.Version = std::to_string(properties.driverVersion);

		Utils::DumpGPUInfo();

		// Create descriptor pools
		Renderer::Submit([]() mutable
		{
			// Create Descriptor Pool
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 10000;
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->DescriptorPools[i]));
				s_Data->DescriptorPoolAllocationCount[i] = 0;
			}

		});

		// Create fullscreen quad
		float x = -1;
		float y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0.0f);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0.0f);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0.0f);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0.0f);
		data[3].TexCoord = glm::vec2(0, 1);

		s_Data->QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data->QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		{
			TextureProperties props;
			props.SamplerWrap = TextureWrap::Clamp;
			s_Data->BRDFLut = Texture2D::Create("Resources/textures/BRDF_LUT.tga", props);
		}

	}

	void VulkanRenderer::Shutdown()
	{
		VulkanShader::ClearUniformBuffers();
		delete s_Data;
	}

	RendererCapabilities& VulkanRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}

	static const std::vector<std::vector<VkWriteDescriptorSet>>& RT_RetrieveOrCreateWriteDescriptors(Ref<UniformBufferSet> uniformBufferSet, Ref<VulkanMaterial> vulkanMaterial)
	{
		size_t shaderHash = vulkanMaterial->GetShader()->GetHash();
		if (s_Data->UniformBufferWriteDescriptorCache.find(uniformBufferSet.Raw()) != s_Data->UniformBufferWriteDescriptorCache.end())
		{
			const auto& shaderMap = s_Data->UniformBufferWriteDescriptorCache.at(uniformBufferSet.Raw());
			if (shaderMap.find(shaderHash) != shaderMap.end())
			{
				const auto& writeDescriptors = shaderMap.at(shaderHash);
				return writeDescriptors;
			}
		}

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Ref<VulkanShader> vulkanShader = vulkanMaterial->GetShader().As<VulkanShader>();
		if (vulkanShader->HasDescriptorSet(0))
		{
			const auto& shaderDescriptorSets = vulkanShader->GetShaderDescriptorSets();
			if (!shaderDescriptorSets.empty())
			{
				for (auto&& [binding, shaderUB] : shaderDescriptorSets[0].UniformBuffers)
				{
					auto& writeDescriptors = s_Data->UniformBufferWriteDescriptorCache[uniformBufferSet.Raw()][shaderHash];
					writeDescriptors.resize(framesInFlight);
					for (uint32_t frame = 0; frame < framesInFlight; frame++)
					{
						Ref<VulkanUniformBuffer> uniformBuffer = uniformBufferSet->Get(binding, 0, frame); // set = 0 for now

						VkWriteDescriptorSet writeDescriptorSet = {};
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writeDescriptorSet.pBufferInfo = &uniformBuffer->GetDescriptorBufferInfo();
						writeDescriptorSet.dstBinding = uniformBuffer->GetBinding();
						writeDescriptors[frame].push_back(writeDescriptorSet);
					}
				}
			}
		}

		return s_Data->UniformBufferWriteDescriptorCache[uniformBufferSet.Raw()][shaderHash];
	}

	void VulkanRenderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, mesh, transform]() mutable
		{
			HY_SCOPE_PERF("VulkanRenderer::RenderMesh");

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<MeshAsset> meshAsset = mesh->GetMeshAsset();
			Ref<VulkanVertexBuffer> vulkanMeshVB = meshAsset->GetVertexBuffer().As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = Ref<VulkanIndexBuffer>(meshAsset->GetIndexBuffer());
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			std::vector<std::vector<VkWriteDescriptorSet>> writeDescriptors;

			auto& materials = mesh->GetMaterials();
			for (auto& material : materials)
			{
				Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
				writeDescriptors = RT_RetrieveOrCreateWriteDescriptors(uniformBufferSet, vulkanMaterial);
				vulkanMaterial->RT_UpdateForRendering(writeDescriptors);
			}

			auto& submeshes = meshAsset->GetSubmeshes();
			for (Submesh& submesh : submeshes)
			{
				auto& material = mesh->GetMaterials()[submesh.MaterialIndex].As<VulkanMaterial>();
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				VkDescriptorSet descriptorSet = material->GetDescriptorSet(frameIndex);

				// NOTE: Descriptor Set 1 is owned by the renderer
				std::array<VkDescriptorSet, 2> descriptorSets = {
					descriptorSet,
					s_Data->ActiveRendererDescriptorSet
				};

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

				glm::mat4 worldTransform = transform * submesh.Transform;

				Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
			}
		});
	}

	void VulkanRenderer::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms)
	{
		HY_CORE_ASSERT(mesh);
		HY_CORE_ASSERT(mesh->GetMeshAsset());

		Buffer pushConstantBuffer;
		pushConstantBuffer.Allocate(sizeof(glm::mat4) + additionalUniforms.Size);
		if (additionalUniforms.Size)
			pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size, sizeof(glm::mat4));

		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, mesh, vulkanMaterial, transform, pushConstantBuffer]() mutable
			{
				HY_SCOPE_PERF("VulkanRenderer::RenderMeshWithMaterial");

				uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

				Ref<MeshAsset> meshAsset = mesh->GetMeshAsset();
				auto vulkanMeshVB = meshAsset->GetVertexBuffer().As<VulkanVertexBuffer>();
				VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

				auto vulkanMeshIB = Ref<VulkanIndexBuffer>(meshAsset->GetIndexBuffer());
				VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
				vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

				const auto& writeDescriptors = RT_RetrieveOrCreateWriteDescriptors(uniformBufferSet, vulkanMaterial);
				vulkanMaterial->RT_UpdateForRendering(writeDescriptors);

				Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				float lineWidth = vulkanPipeline->GetSpecification().LineWidth;
				if (lineWidth != 1.0f)
					vkCmdSetLineWidth(commandBuffer, lineWidth);

				// Bind descriptor sets describing shader binding points
				VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);
				if (descriptorSet)
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

				Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
				if (uniformStorageBuffer)
					vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantBuffer.Size, uniformStorageBuffer.Size, uniformStorageBuffer.Data);

			auto& submeshes = meshAsset->GetSubmeshes();
			for (Submesh& submesh : submeshes)
			{
				glm::mat4 worldTransform = transform * submesh.Transform;
				pushConstantBuffer.Write(&worldTransform, sizeof(glm::mat4));
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, pushConstantBuffer.Size, pushConstantBuffer.Data);
				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
			}
			pushConstantBuffer.Release();
		});
	}

	void VulkanRenderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, const glm::mat4& transform)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, vulkanMaterial, transform]() mutable
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_Data->QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_Data->QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			
			const auto& writeDescriptors = RT_RetrieveOrCreateWriteDescriptors(uniformBufferSet, vulkanMaterial);
			vulkanMaterial->RT_UpdateForRendering(writeDescriptors);

			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
			VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
			if (descriptorSet)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
			vkCmdDrawIndexed(commandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount /*= 0*/)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		if (indexCount == 0)
			indexCount = indexBuffer->GetCount();

		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, vulkanMaterial, vertexBuffer, indexBuffer, transform, indexCount]() mutable
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = vertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = indexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			const auto& writeDescriptors = RT_RetrieveOrCreateWriteDescriptors(uniformBufferSet, vulkanMaterial);
			vulkanMaterial->RT_UpdateForRendering(writeDescriptors);

			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
			VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
			if (descriptorSet)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
			if (uniformStorageBuffer)
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);

			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		});
	}

	VkDescriptorSet VulkanRenderer::RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo)
	{
		uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
		allocInfo.descriptorPool = s_Data->DescriptorPools[bufferIndex];
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VkDescriptorSet result;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result));
		s_Data->DescriptorPoolAllocationCount[bufferIndex] += allocInfo.descriptorSetCount;
		return result;
	}

	void VulkanRenderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, vulkanMaterial]() mutable
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_Data->QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_Data->QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			if (uniformBufferSet)
			{
				const auto& writeDescriptors = RT_RetrieveOrCreateWriteDescriptors(uniformBufferSet, vulkanMaterial);
				vulkanMaterial->RT_UpdateForRendering(writeDescriptors);
			}
			else
			{
				vulkanMaterial->RT_UpdateForRendering();
			}

			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
			VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
			if (descriptorSet)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
			if (uniformStorageBuffer.Size)
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);

			vkCmdDrawIndexed(commandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow)
	{
		if (!environment)
			environment = Renderer::GetEmptyEnvironment();

		Renderer::Submit([sceneRenderer, environment, shadow]() mutable
		{
			auto shader = Renderer::GetShaderLibrary()->Get("PBR_Static");
			Ref<VulkanShader> pbrShader = shader.As<VulkanShader>();
			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();

			if (s_Data->RendererDescriptorSet.find(sceneRenderer.Raw()) == s_Data->RendererDescriptorSet.end())
			{
				uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				s_Data->RendererDescriptorSet[sceneRenderer.Raw()].resize(framesInFlight);
				for (uint32_t i = 0; i < framesInFlight; i++)
					s_Data->RendererDescriptorSet.at(sceneRenderer.Raw())[i] = pbrShader->CreateDescriptorSets(1);

			}

			VkDescriptorSet descriptorSet = s_Data->RendererDescriptorSet.at(sceneRenderer.Raw())[bufferIndex].DescriptorSets[0];
			s_Data->ActiveRendererDescriptorSet = descriptorSet;

			std::array<VkWriteDescriptorSet, 4> writeDescriptors;

			Ref<VulkanTextureCube> radianceMap = environment->RadianceMap.As<VulkanTextureCube>();
			Ref<VulkanTextureCube> irradianceMap = environment->IrradianceMap.As<VulkanTextureCube>();
		
			writeDescriptors[0] = *pbrShader->GetDescriptorSet("u_EnvRadianceTex", 1);
			writeDescriptors[0].dstSet = descriptorSet;
			const auto& radianceMapImageInfo = radianceMap->GetVulkanDescriptorInfo();
			writeDescriptors[0].pImageInfo = &radianceMapImageInfo;

			writeDescriptors[1] = *pbrShader->GetDescriptorSet("u_EnvIrradianceTex", 1);
			writeDescriptors[1].dstSet = descriptorSet;
			const auto& irradianceMapImageInfo = irradianceMap->GetVulkanDescriptorInfo();
			writeDescriptors[1].pImageInfo = &irradianceMapImageInfo;

			writeDescriptors[2] = *pbrShader->GetDescriptorSet("u_BRDFLUTTexture", 1);
			writeDescriptors[2].dstSet = descriptorSet;
			const auto& brdfLutImageInfo = s_Data->BRDFLut.As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
			writeDescriptors[2].pImageInfo = &brdfLutImageInfo;

			writeDescriptors[3] = *pbrShader->GetDescriptorSet("u_ShadowMapTexture", 1);
			writeDescriptors[3].dstSet = descriptorSet;
			const auto& shadowImageInfo = shadow.As<VulkanImage2D>()->GetDescriptor();
			writeDescriptors[3].pImageInfo = &shadowImageInfo;

			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		});
	}

	void VulkanRenderer::BeginFrame()
	{
		Renderer::Submit([]()
		{
			VulkanSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();

			// Reset descriptor pools here
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			uint32_t bufferIndex = swapChain.GetCurrentBufferIndex();
			vkResetDescriptorPool(device, s_Data->DescriptorPools[bufferIndex], 0);
			memset(s_Data->DescriptorPoolAllocationCount.data(), 0, s_Data->DescriptorPoolAllocationCount.size() * sizeof(uint32_t));

		});
	}

	void VulkanRenderer::EndFrame()
	{
	}

	void VulkanRenderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass, bool explicitClear)
	{
		Renderer::Submit([renderCommandBuffer, renderPass, explicitClear]()
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			auto fb = renderPass->GetSpecification().TargetFramebuffer;
			Ref<VulkanFramebuffer> framebuffer = fb.As<VulkanFramebuffer>();
			const auto& fbSpec = framebuffer->GetSpecification();

			uint32_t width = framebuffer->GetWidth();
			uint32_t height = framebuffer->GetHeight();

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;

			// TODO: Does our framebuffer have a depth attachment?

			const auto& clearValues = framebuffer->GetVulkanClearValues();
			
			renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
			renderPassBeginInfo.pClearValues = clearValues.data();
			renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			if (explicitClear)
			{
				uint32_t colorAttachmentCount = (uint32_t)framebuffer->GetColorAttachmentCount();
				uint32_t totalAttachmentCount = colorAttachmentCount + (framebuffer->HasDepthAttachment() ? 1 : 0);
				HY_CORE_ASSERT(clearValues.size() == totalAttachmentCount);

				std::vector<VkClearAttachment> attachments(totalAttachmentCount);
				std::vector<VkClearRect> clearRects(totalAttachmentCount);
				for (uint32_t i = 0; i < colorAttachmentCount; i++)
				{
					attachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					attachments[i].colorAttachment = i;
					attachments[i].clearValue = clearValues[i];

					clearRects[i].rect.offset = { (int32_t)0, (int32_t)0 };
					clearRects[i].rect.extent = { width, height };
					clearRects[i].baseArrayLayer = 0;
					clearRects[i].layerCount = 1;
				}

				if (framebuffer->HasDepthAttachment())
				{
					attachments[colorAttachmentCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
					attachments[colorAttachmentCount].clearValue = clearValues[colorAttachmentCount];
					clearRects[colorAttachmentCount].rect.offset = { (int32_t)0, (int32_t)0 };
					clearRects[colorAttachmentCount].rect.extent = { width, height };
					clearRects[colorAttachmentCount].baseArrayLayer = 0;
					clearRects[colorAttachmentCount].layerCount = 1;
				}

				vkCmdClearAttachments(commandBuffer, totalAttachmentCount, attachments.data(), totalAttachmentCount, clearRects.data());
			}

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		});
	}

	void VulkanRenderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		Renderer::Submit([renderCommandBuffer]()
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			vkCmdEndRenderPass(commandBuffer);
		});
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> VulkanRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		if (!Renderer::GetConfig().ComputeEnvironmentMaps)
			return { Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture() };

		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> envEquirect = Texture2D::Create(filepath);
		HY_CORE_ASSERT(envEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");

		Ref<TextureCube> envUnfiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
		Ref<TextureCube> envFiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
		
		// Convert equirectangular to cubemap
		Ref<Shader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		Ref<VulkanComputePipeline> equirectangularConversionPipeline = Ref<VulkanComputePipeline>::Create(equirectangularConversionShader);

		Renderer::Submit([equirectangularConversionPipeline, envEquirect, envUnfiltered, cubemapSize]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = equirectangularConversionPipeline->GetShader();

			std::array<VkWriteDescriptorSet, 2> writeDescriptors;
			auto descriptorSet = shader->CreateDescriptorSets();
			Ref<VulkanTextureCube> envUnfilteredCubemap = envUnfiltered.As<VulkanTextureCube>();
			writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

			Ref<VulkanTexture2D> envEquirectVK = envEquirect.As<VulkanTexture2D>();
			writeDescriptors[1] = *shader->GetDescriptorSet("u_EquirectangularTex");
			writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[1].pImageInfo = &envEquirectVK->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
			equirectangularConversionPipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), cubemapSize / 32, cubemapSize / 32, 6);

			envUnfilteredCubemap->GenerateMips(true);
		});

		Ref<Shader> environmentMipFilterShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter");
		Ref<VulkanComputePipeline> environmentMipFilterPipeline = Ref<VulkanComputePipeline>::Create(environmentMipFilterShader);

		Renderer::Submit([environmentMipFilterPipeline, envUnfiltered, envFiltered, cubemapSize]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = environmentMipFilterPipeline->GetShader();

			Ref<VulkanTextureCube> envFilteredCubemap = envFiltered.As<VulkanTextureCube>();
			VkDescriptorImageInfo imageInfo = envFilteredCubemap->GetVulkanDescriptorInfo();

			uint32_t mipCount = Utils::CalculateMipCount(cubemapSize, cubemapSize);

			std::vector<VkWriteDescriptorSet> writeDescriptors(mipCount * 2);
			std::vector<VkDescriptorImageInfo> mipImageInfos(mipCount);
			auto descriptorSet = shader->CreateDescriptorSets(0, 12);
			for (uint32_t i = 0; i < mipCount; i++)
			{
				VkDescriptorImageInfo& mipImageInfo = mipImageInfos[i];
				mipImageInfo = imageInfo;
				mipImageInfo.imageView = envFilteredCubemap->CreateImageViewSingleMip(i);

				writeDescriptors[i * 2 + 0] = *shader->GetDescriptorSet("outputTexture");
				writeDescriptors[i * 2 + 0].dstSet = descriptorSet.DescriptorSets[i]; // Should this be set inside the shader?
				writeDescriptors[i * 2 + 0].pImageInfo = &mipImageInfo;

				Ref<VulkanTextureCube> envUnfilteredCubemap = envUnfiltered.As<VulkanTextureCube>();
				writeDescriptors[i * 2 + 1] = *shader->GetDescriptorSet("inputTexture");
				writeDescriptors[i * 2 + 1].dstSet = descriptorSet.DescriptorSets[i]; // Should this be set inside the shader?
				writeDescriptors[i * 2 + 1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
			}

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

			environmentMipFilterPipeline->Begin(); // begin compute pass
			const float deltaRoughness = 1.0f / glm::max((float)envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
			for (uint32_t i = 0, size = cubemapSize; i < mipCount; i++, size /= 2)
			{
				uint32_t numGroups = glm::max(1u, size / 32);
				float roughness = i * deltaRoughness;
				roughness = glm::max(roughness, 0.05f);
				environmentMipFilterPipeline->SetPushConstants(&roughness, sizeof(float));
				environmentMipFilterPipeline->Dispatch(descriptorSet.DescriptorSets[i], numGroups, numGroups, 6);
			}
			environmentMipFilterPipeline->End();
		});

		Ref<Shader> environmentIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance");
		Ref<VulkanComputePipeline> environmentIrradiancePipeline = Ref<VulkanComputePipeline>::Create(environmentIrradianceShader);
		Ref<TextureCube> irradianceMap = TextureCube::Create(ImageFormat::RGBA32F, irradianceMapSize, irradianceMapSize);

		Renderer::Submit([environmentIrradiancePipeline, irradianceMap, envFiltered]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = environmentIrradiancePipeline->GetShader();

			Ref<VulkanTextureCube> envFilteredCubemap = envFiltered.As<VulkanTextureCube>();
			Ref<VulkanTextureCube> irradianceCubemap = irradianceMap.As<VulkanTextureCube>();
			auto descriptorSet = shader->CreateDescriptorSets();

			std::array<VkWriteDescriptorSet, 2> writeDescriptors;
			writeDescriptors[0] = *shader->GetDescriptorSet("o_IrradianceMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0];
			writeDescriptors[0].pImageInfo = &irradianceCubemap->GetVulkanDescriptorInfo();

			writeDescriptors[1] = *shader->GetDescriptorSet("u_RadianceMap");
			writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0];
			writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
			environmentIrradiancePipeline->Begin();
			environmentIrradiancePipeline->SetPushConstants(&Renderer::GetConfig().IrradianceMapComputeSamples, sizeof(uint32_t));
			environmentIrradiancePipeline->Dispatch(descriptorSet.DescriptorSets[0], irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
			environmentIrradiancePipeline->End();

			irradianceCubemap->GenerateMips();
		});

		return { envFiltered, irradianceMap };
	}

	Ref<TextureCube> VulkanRenderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<TextureCube> environmentMap = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);

		Ref<Shader> preethamSkyShader = Renderer::GetShaderLibrary()->Get("PreethamSky");
		Ref<VulkanComputePipeline> preethamSkyComputePipeline = Ref<VulkanComputePipeline>::Create(preethamSkyShader);

		glm::vec3 params = { turbidity, azimuth, inclination };
		Renderer::Submit([preethamSkyComputePipeline, environmentMap, cubemapSize, params]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = preethamSkyComputePipeline->GetShader();

			std::array<VkWriteDescriptorSet, 1> writeDescriptors;
			auto descriptorSet = shader->CreateDescriptorSets();
			Ref<VulkanTextureCube> envUnfilteredCubemap = environmentMap.As<VulkanTextureCube>();
			writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

			preethamSkyComputePipeline->Begin();
			preethamSkyComputePipeline->SetPushConstants(&params, sizeof(glm::vec3));
			preethamSkyComputePipeline->Dispatch(descriptorSet.DescriptorSets[0], cubemapSize / 32, cubemapSize / 32, 6);
			preethamSkyComputePipeline->End();

			envUnfilteredCubemap->GenerateMips(true);
		});

		return environmentMap;
	}

}