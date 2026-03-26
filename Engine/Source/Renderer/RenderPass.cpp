#include "vapch.hpp"
#include "RenderPass.hpp"

#include "Renderer.hpp"

#include "Renderer/Backend/OpenGL/OpenGLRenderPass.hpp"

namespace Vanta {

    Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
    {
        switch (RendererAPI::Current())
        {
            case RendererAPIType::None:    VA_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
            case RendererAPIType::OpenGL:  return std::make_shared<OpenGLRenderPass>(spec);
        }

        VA_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

}