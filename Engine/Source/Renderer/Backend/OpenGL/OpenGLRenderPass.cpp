#include "vapch.hpp"
#include "OpenGLRenderPass.hpp"

namespace Vanta {

    OpenGLRenderPass::OpenGLRenderPass(const RenderPassSpecification& spec)
        : m_Specification(spec)
    {
    }

    OpenGLRenderPass::~OpenGLRenderPass()
    {
    }

}