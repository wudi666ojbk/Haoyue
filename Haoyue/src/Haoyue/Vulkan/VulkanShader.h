#pragma once

#include "Haoyue/Renderer/Shader.h"

#include "Vulkan.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace Haoyue {

	class VulkanShader : public Shader
	{
	public:
		struct UniformBuffer
		{
			VkDescriptorBufferInfo Descriptor;
			uint32_t Size = 0;
			uint32_t BindingPoint = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		};

		struct ImageSampler
		{
			uint32_t BindingPoint = 0;
			uint32_t DescriptorSet = 0;
			std::string Name;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		};

		struct PushConstantRange
		{
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			uint32_t Offset = 0;
			uint32_t Size = 0;
		};
	public:
		VulkanShader(const std::string& path, bool forceCompile);
		virtual ~VulkanShader();

		void Reload(bool forceCompile = false) override;

		virtual size_t GetHash() const override;

		virtual const std::string& GetName() const override { return  m_Name; }
		virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const override { return m_Buffers; }
		virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const override;
		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

		// Vulkan-specific
		const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() const { return m_PipelineShaderStageCreateInfos; }

		VkDescriptorSet GetDescriptorSet() { return m_DescriptorSet; }
		VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t set) { return m_DescriptorSetLayouts.at(set); }
		std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayouts();

		UniformBuffer& GetUniformBuffer(uint32_t binding = 0, uint32_t set = 0) { HY_CORE_ASSERT(m_ShaderDescriptorSets.at(set).UniformBuffers.size() > binding); return *m_ShaderDescriptorSets.at(set).UniformBuffers.at(binding); }
		uint32_t GetUniformBufferCount(uint32_t set = 0)
		{
			if (m_ShaderDescriptorSets.size() < set)
				return 0;

			return m_ShaderDescriptorSets[set].UniformBuffers.size();
		}

		struct ShaderDescriptorSet
		{
			std::unordered_map<uint32_t, UniformBuffer*> UniformBuffers;
			std::unordered_map<uint32_t, ImageSampler> ImageSamplers;
			std::unordered_map<uint32_t, ImageSampler> StorageImages;

			std::unordered_map<std::string, VkWriteDescriptorSet> WriteDescriptorSets;

			operator bool() const { return !(UniformBuffers.empty() && ImageSamplers.empty() && StorageImages.empty()); }
		};
		const std::vector<ShaderDescriptorSet>& GetShaderDescriptorSets() const { return m_ShaderDescriptorSets; }
		bool HasDescriptorSet(uint32_t set) const { return m_TypeCounts.find(set) != m_TypeCounts.end(); }

		const std::vector<PushConstantRange>& GetPushConstantRanges() const { return m_PushConstantRanges; }

		struct ShaderMaterialDescriptorSet
		{
			VkDescriptorPool Pool = nullptr;
			std::vector<VkDescriptorSet> DescriptorSets;
		};

		ShaderMaterialDescriptorSet AllocateDescriptorSet(uint32_t set = 0);
		ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set = 0);
		ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set, uint32_t numberOfSets);
		const VkWriteDescriptorSet* GetDescriptorSet(const std::string& name, uint32_t set = 0) const;

		static void ClearUniformBuffers();
	private:
		std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& source);
		void CompileOrGetVulkanBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile);
		void LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void Reflect(VkShaderStageFlagBits shaderStage, const std::vector<uint32_t>& shaderData);
		void ReflectAllShaderStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);

		void CreateDescriptors();
	private:
		std::vector<VkPipelineShaderStageCreateInfo> m_PipelineShaderStageCreateInfos;
		std::unordered_map<VkShaderStageFlagBits, std::string> m_ShaderSource;
		std::string m_AssetPath;
		std::string m_Name;

		std::vector<ShaderDescriptorSet> m_ShaderDescriptorSets;

		std::vector<PushConstantRange> m_PushConstantRanges;
		std::unordered_map<std::string, ShaderResourceDeclaration> m_Resources;

		std::unordered_map<std::string, ShaderBuffer> m_Buffers;

		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		VkDescriptorSet m_DescriptorSet;

		std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> m_TypeCounts;
	};

}
