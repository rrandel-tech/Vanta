#include "vapch.hpp"
#include "RenderCommandBuffer.hpp"

#include "Renderer/Backend/Vulkan/VulkanRenderCommandBuffer.hpp"

#include "Renderer/RendererAPI.hpp"

namespace Vanta {

    Ref<RenderCommandBuffer> RenderCommandBuffer::Create(uint32_t count)
    {
        switch (RendererAPI::Current())
        {
            case RendererAPIType::None:    return nullptr;
            case RendererAPIType::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(count);
        }
        VA_CORE_ASSERT(false, "Unknown RendererAPI");
        return nullptr;
    }

}