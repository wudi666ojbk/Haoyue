#include "pch.h"
#include "ImGui.h"

#include "Haoyue/Renderer/RendererAPI.h"

#include "Haoyue/Vulkan/VulkanTexture.h"
#include "Haoyue/Vulkan/VulkanImage.h"

#include "imgui/examples/imgui_impl_vulkan_with_textures.h"

namespace Haoyue::UI {

	void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
		const auto& imageInfo = vulkanImage->GetImageInfo();
		if (!imageInfo.ImageView)
			return;
		auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
		ImGui::Image((ImTextureID)textureID, size, uv0, uv1, tint_col, border_col);
	}

	void Image(const Ref<Image2D>& image, uint32_t layer, const ImVec2& size, const ImVec2 uv0, const ImVec2 uv1, const ImVec4 tint_col, const ImVec4 border_col)
	{
		Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
		auto imageInfo = vulkanImage->GetImageInfo();
		imageInfo.ImageView = vulkanImage->GetLayerImageView(layer);
		if (!imageInfo.ImageView)
			return;
		auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
		ImGui::Image((ImTextureID)textureID, size, uv0, uv1, tint_col, border_col);
	}

	void Image(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		Ref<VulkanTexture2D> vulkanTexture = texture.As<VulkanTexture2D>();
		const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetVulkanDescriptorInfo();
		auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
		ImGui::Image((ImTextureID)textureID, size, uv0, uv1, tint_col, border_col);
	}

	bool ImageButton(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImageButton(nullptr, image, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool ImageButton(const char* stringID, const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
		const auto& imageInfo = vulkanImage->GetImageInfo();
		if (!imageInfo.ImageView)
			return false;
		auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
		ImGuiID id = (ImGuiID)((((uint64_t)imageInfo.ImageView) >> 32) ^ (uint32_t)imageInfo.ImageView);
		if (stringID)
		{
			ImGuiID strID = ImGui::GetID(stringID);
			id = id ^ strID;
		}
		return ImGui::ImageButton((ImTextureID)textureID, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImageButton(nullptr, texture, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool ImageButton(const char* stringID, const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		Ref<VulkanTexture2D> vulkanTexture = texture.As<VulkanTexture2D>();
		const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetVulkanDescriptorInfo();
		auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
		ImGuiID id = (ImGuiID)((((uint64_t)imageInfo.imageView) >> 32) ^ (uint32_t)imageInfo.imageView);
		if (stringID)
		{
			ImGuiID strID = ImGui::GetID(stringID);
			id = id ^ strID;
		}
		return ImGui::ImageButton((ImTextureID)textureID, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

}