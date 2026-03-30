#include "vapch.hpp"
#include "ImGui.hpp"

#include "Renderer/RendererAPI.hpp"

#include "Renderer/Backend/Vulkan/VulkanTexture.hpp"
#include "Renderer/Backend/Vulkan/VulkanImage.hpp"

#include "Renderer/Backend/OpenGL/OpenGLTexture.hpp"
#include "Renderer/Backend/OpenGL/OpenGLImage.hpp"

#include "backends/imgui_impl_vulkan.h"

#include <unordered_map>

namespace Vanta::UI {

    static std::unordered_map<VkImageView, ImTextureID> s_VulkanTextureCache;

    inline ImTextureID GetVulkanTextureID(const VkDescriptorImageInfo& imageInfo)
    {
        auto it = s_VulkanTextureCache.find(imageInfo.imageView);
        if (it != s_VulkanTextureCache.end())
            return it->second;

        VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
        ImTextureID id = (ImTextureID)descriptorSet;
        s_VulkanTextureCache[imageInfo.imageView] = id;
        return id;
    }

    void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
    {
        if (RendererAPI::Current() == RendererAPIType::OpenGL)
        {
            Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
            ImGui::Image((ImTextureID)glImage->GetRendererID(), size, uv0, uv1, tint_col, border_col);
        }
        else
        {
            Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
            const auto& imageInfo = vulkanImage->GetImageInfo();
            if (!imageInfo.ImageView)
                return;
            ImTextureID textureID = GetVulkanTextureID(vulkanImage->GetDescriptor());
            ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
        }
    }

    void Image(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
    {
        if (RendererAPI::Current() == RendererAPIType::OpenGL)
        {
            Ref<OpenGLImage2D> image = texture->GetImage().As<OpenGLImage2D>();
            ImGui::Image((ImTextureID)image->GetRendererID(), size, uv0, uv1, tint_col, border_col);
        }
        else
        {
            Ref<VulkanTexture2D> vulkanTexture = texture.As<VulkanTexture2D>();
            const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetVulkanDescriptorInfo();
            ImTextureID textureID = GetVulkanTextureID(imageInfo);
            ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
        }
    }

    bool ImageButton(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bg_col, const ImVec4& tint_col)
    {
        if (RendererAPI::Current() == RendererAPIType::OpenGL)
        {
            Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
            return ImGui::ImageButton("##imgbtn", (ImTextureID)glImage->GetRendererID(), size, uv0, uv1, bg_col, tint_col);
        }
        else
        {
            Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
            const auto& imageInfo = vulkanImage->GetImageInfo();
            if (!imageInfo.ImageView)
                return false;
            ImTextureID textureID = GetVulkanTextureID(vulkanImage->GetDescriptor());
            return ImGui::ImageButton("##imgbtn", textureID, size, uv0, uv1, bg_col, tint_col);
        }
    }

    bool ImageButton(const Ref<Texture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bg_col, const ImVec4& tint_col)
    {
        if (RendererAPI::Current() == RendererAPIType::OpenGL)
        {
            Ref<OpenGLImage2D> image = texture->GetImage().As<OpenGLImage2D>();
            return ImGui::ImageButton("##imgbtn", (ImTextureID)image->GetRendererID(), size, uv0, uv1, bg_col, tint_col);
        }
        else
        {
            Ref<VulkanTexture2D> vulkanTexture = texture.As<VulkanTexture2D>();
            const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetVulkanDescriptorInfo();
            ImTextureID textureID = GetVulkanTextureID(imageInfo);
            return ImGui::ImageButton("##imgbtn", textureID, size, uv0, uv1, bg_col, tint_col);
        }
    }

    void ClearVulkanTextureCache() { s_VulkanTextureCache.clear(); }

}