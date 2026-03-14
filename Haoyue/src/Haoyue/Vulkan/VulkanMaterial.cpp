#include "pch.h"
#include "VulkanMaterial.h"

#include "Haoyue/Renderer/Renderer.h"

#include "VulkanContext.h"
#include "VulkanTexture.h"
#include "VulkanImage.h"
#include "VulkanPipeline.h"
#include "VulkanUniformBuffer.h"

namespace Haoyue {

	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const std::string& name)
		: m_Shader(shader), m_Name(name)
	{
		Init();
		Renderer::RegisterShaderDependency(shader, this);
	}

	VulkanMaterial::~VulkanMaterial()
	{
	}

	void VulkanMaterial::Init()
	{
		AllocateStorage();

		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;

		Invalidate();

#if INVALIDATE_ON_RT
		Ref<VulkanMaterial> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->Invalidate();
			});
#endif
	}

	void VulkanMaterial::Invalidate()
	{
		auto shader = m_Shader.As<VulkanShader>();
		if (shader->HasDescriptorSet(0))
		{
			const auto& shaderDescriptorSets = shader->GetShaderDescriptorSets();
			if (!shaderDescriptorSets.empty())
			{
				m_DescriptorSet = shader->CreateDescriptorSets();

				Ref<VulkanShader> vulkanShader = m_Shader.As<VulkanShader>();
				for (auto&& [binding, shaderUB] : shaderDescriptorSets[0].UniformBuffers)
				{
					Ref<VulkanUniformBuffer> uniformBuffer = Renderer::GetUniformBuffer(binding, 0).As<VulkanUniformBuffer>(); // set = 0 for now

					VkWriteDescriptorSet writeDescriptorSet = {};
					writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSet.descriptorCount = 1;
					writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					writeDescriptorSet.pBufferInfo = &uniformBuffer->GetDescriptorBufferInfo();
					writeDescriptorSet.dstBinding = uniformBuffer->GetBinding();
					m_WriteDescriptors.push_back(writeDescriptorSet);
				}

				for (auto&& [binding, descriptor] : m_ResidentDescriptors)
					m_PendingDescriptors.push_back(descriptor);
			}
		}
	}

	void VulkanMaterial::AllocateStorage()
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		if (shaderBuffers.size() > 0)
		{
			uint32_t size = 0;
			for (auto [name, shaderBuffer] : shaderBuffers)
				size += shaderBuffer.Size;

			m_UniformStorageBuffer.Allocate(size);
			m_UniformStorageBuffer.ZeroInitialize();
		}
	}

	void VulkanMaterial::OnShaderReloaded()
	{
		return;
		AllocateStorage();
	}

	const ShaderUniform* VulkanMaterial::FindUniformDeclaration(const std::string& name)
	{
		const auto& shaderBuffers = m_Shader->GetShaderBuffers();

		HY_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

		if (shaderBuffers.size() > 0)
		{
			const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
			if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
				return nullptr;

			return &buffer.Uniforms.at(name);
		}
		return nullptr;
	}

	const ShaderResourceDeclaration* VulkanMaterial::FindResourceDeclaration(const std::string& name)
	{
		auto& resources = m_Shader->GetResources();
		for (const auto& [n, resource] : resources)
		{
			if (resource.GetName() == name)
				return &resource;
		}
		return nullptr;
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture)
	{
		const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
		HY_CORE_ASSERT(resource);

		uint32_t binding = resource->GetRegister();
		// Texture is already set
		if (binding < m_Textures.size() && m_Textures[binding] && texture->GetHash() == m_Textures[binding]->GetHash())
			return;

		if (binding >= m_Textures.size())
			m_Textures.resize(binding + 1);
		m_Textures[binding] = texture;

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		HY_CORE_ASSERT(wds);
		m_ResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::Texture2D, *wds, {}, texture.As<Texture>(), nullptr });
		m_PendingDescriptors.push_back(m_ResidentDescriptors.at(binding));
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<TextureCube>& texture)
	{
		const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
		HY_CORE_ASSERT(resource);

		uint32_t binding = resource->GetRegister();
		// Texture is already set
		if (binding < m_Textures.size() && m_Textures[binding] && texture->GetHash() == m_Textures[binding]->GetHash())
			return;

		if (binding >= m_Textures.size())
			m_Textures.resize(binding + 1);
		m_Textures[binding] = texture;

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		HY_CORE_ASSERT(wds);
		m_ResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::TextureCube, *wds, {}, texture.As<Texture>(), nullptr });
		m_PendingDescriptors.push_back(m_ResidentDescriptors.at(binding));
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const VkDescriptorImageInfo& imageInfo)
	{
		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		HY_CORE_ASSERT(wds);

		if (m_ImageInfos.find(name) != m_ImageInfos.end())
		{
			if (m_ImageInfos.at(name).imageView == imageInfo.imageView)
				return;
		}

		m_ImageInfos[name] = imageInfo;

		VkWriteDescriptorSet descriptorSet = *wds;
		descriptorSet.pImageInfo = &m_ImageInfos.at(name);
		m_WriteDescriptors.push_back(descriptorSet);
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image)
	{
		HY_CORE_ASSERT(image.As<VulkanImage2D>()->GetImageInfo().ImageView, "ImageView is null");

		const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
		HY_CORE_ASSERT(resource);

		uint32_t binding = resource->GetRegister();
		// TODO: replace with set/map
		if (binding < m_Images.size() && m_Images[binding] && m_ImageHashes.at(binding) == image->GetHash())
			return;

		if (resource->GetRegister() >= m_Images.size())
			m_Images.resize(resource->GetRegister() + 1);
		m_Images[resource->GetRegister()] = image;
		m_ImageHashes[resource->GetRegister()] = image->GetHash();

		const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
		HY_CORE_ASSERT(wds);
		m_ResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::Image2D, *wds, {}, nullptr, image.As<Image>() });
		m_PendingDescriptors.push_back(m_ResidentDescriptors.at(binding));
	}

	void VulkanMaterial::Set(const std::string& name, float value)
	{
		Set<float>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, int value)
	{
		Set<int>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, uint32_t value)
	{
		Set<uint32_t>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, bool value)
	{
		// Bools are 4-byte ints
		Set<int>(name, (int)value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec2& value)
	{
		Set<glm::vec2>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec3& value)
	{
		Set<glm::vec3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec4& value)
	{
		Set<glm::vec4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::mat3& value)
	{
		Set<glm::mat3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::mat4& value)
	{
		Set<glm::mat4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<Texture2D>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<TextureCube>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<Image2D>& image)
	{
		SetVulkanDescriptor(name, image);
	}

	float& VulkanMaterial::GetFloat(const std::string& name)
	{
		return Get<float>(name);
	}

	int32_t& VulkanMaterial::GetInt(const std::string& name)
	{
		return Get<int32_t>(name);
	}

	uint32_t& VulkanMaterial::GetUInt(const std::string& name)
	{
		return Get<uint32_t>(name);
	}

	bool& VulkanMaterial::GetBool(const std::string& name)
	{
		return Get<bool>(name);
	}

	glm::vec2& VulkanMaterial::GetVector2(const std::string& name)
	{
		return Get<glm::vec2>(name);
	}

	glm::vec3& VulkanMaterial::GetVector3(const std::string& name)
	{
		return Get<glm::vec3>(name);
	}

	glm::vec4& VulkanMaterial::GetVector4(const std::string& name)
	{
		return Get<glm::vec4>(name);
	}

	glm::mat3& VulkanMaterial::GetMatrix3(const std::string& name)
	{
		return Get<glm::mat3>(name);
	}

	glm::mat4& VulkanMaterial::GetMatrix4(const std::string& name)
	{
		return Get<glm::mat4>(name);
	}

	Ref<Texture2D> VulkanMaterial::GetTexture2D(const std::string& name)
	{
		return GetResource<Texture2D>(name);
	}

	Ref<TextureCube> VulkanMaterial::TryGetTextureCube(const std::string& name)
	{
		return TryGetResource<TextureCube>(name);
	}

	Ref<Texture2D> VulkanMaterial::TryGetTexture2D(const std::string& name)
	{
		return TryGetResource<Texture2D>(name);
	}

	Ref<TextureCube> VulkanMaterial::GetTextureCube(const std::string& name)
	{
		return GetResource<TextureCube>(name);
	}

	void VulkanMaterial::UpdateForRendering()
	{
		Ref<VulkanMaterial> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->RT_UpdateForRendering();
			});
	}

	void VulkanMaterial::RT_UpdateForRendering()
	{
		auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		for (auto&& [binding, descriptor] : m_ResidentDescriptors)
		{
			if (descriptor->Type == PendingDescriptorType::Image2D)
			{
				Ref<VulkanImage2D> image = descriptor->Image.As<VulkanImage2D>();
				HY_CORE_ASSERT(image->GetImageInfo().ImageView, "ImageView is null");
				if (descriptor->WDS.pImageInfo && image->GetImageInfo().ImageView != descriptor->WDS.pImageInfo->imageView)
				{
					HY_CORE_WARN("Found out of date Image2D descriptor ({0} vs. {1})", (void*)image->GetImageInfo().ImageView, (void*)descriptor->WDS.pImageInfo->imageView);
					m_PendingDescriptors.emplace_back(descriptor);
				}
			}
		}

		for (auto& pd : m_PendingDescriptors)
		{
			if (pd->Type == PendingDescriptorType::Texture2D)
			{
				Ref<VulkanTexture2D> texture = pd->Texture.As<VulkanTexture2D>();
				pd->ImageInfo = texture->GetVulkanDescriptorInfo();
				pd->WDS.pImageInfo = &pd->ImageInfo;
			}
			else if (pd->Type == PendingDescriptorType::TextureCube)
			{
				Ref<VulkanTextureCube> texture = pd->Texture.As<VulkanTextureCube>();
				pd->ImageInfo = texture->GetVulkanDescriptorInfo();
				pd->WDS.pImageInfo = &pd->ImageInfo;
			}
			else if (pd->Type == PendingDescriptorType::Image2D)
			{
				Ref<VulkanImage2D> image = pd->Image.As<VulkanImage2D>();
				pd->ImageInfo = image->GetDescriptor();
				pd->WDS.pImageInfo = &pd->ImageInfo;
			}

			m_WriteDescriptors.push_back(pd->WDS);
		}

		if (m_WriteDescriptors.size())
		{
			for (auto& writeDescriptor : m_WriteDescriptors)
				writeDescriptor.dstSet = m_DescriptorSet.DescriptorSets[0];

			HY_CORE_WARN("VulkanMaterial - Updating {0} descriptor sets", m_WriteDescriptors.size());
			vkUpdateDescriptorSets(vulkanDevice, m_WriteDescriptors.size(), m_WriteDescriptors.data(), 0, nullptr);
		}

		m_PendingDescriptors.clear();
		m_WriteDescriptors.clear();
	}

}