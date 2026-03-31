#pragma once

#include "Core/Assert.hpp"
#include "Renderer/RenderCommandBuffer.hpp"
#include "vulkan/vulkan.h"

namespace Vanta {

    class VulkanRenderCommandBuffer : public RenderCommandBuffer
    {
    public:
        VulkanRenderCommandBuffer(uint32_t count = 0);
        ~VulkanRenderCommandBuffer();

        virtual void Begin() override;
        virtual void End() override;
        virtual void Submit() override;

        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
        {
            VA_CORE_ASSERT(frameIndex < m_CommandBuffers.size());
            return m_CommandBuffers[frameIndex];
        }
    private:
        VkCommandPool m_CommandPool = nullptr;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkFence> m_WaitFences;

        int m_ActiveBufferIndex = -1;
    };

}