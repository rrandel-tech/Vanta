#include "vapch.hpp"
#include "UniformBuffer.hpp"

#include "Renderer/Renderer.hpp"

#include "Renderer/Backend/OpenGL/OpenGLUniformBuffer.hpp"
#include "Renderer/Backend/Vulkan/VulkanUniformBuffer.hpp"

#include "Renderer/RendererAPI.hpp"

namespace Vanta {

    Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
    {
        switch (RendererAPI::Current())
        {
            case RendererAPIType::None:     return nullptr;
            case RendererAPIType::Vulkan:  return Ref<VulkanUniformBuffer>::Create(size, binding);
            case RendererAPIType::OpenGL:  return Ref<OpenGLUniformBuffer>::Create(size, binding);
        }

        VA_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

}