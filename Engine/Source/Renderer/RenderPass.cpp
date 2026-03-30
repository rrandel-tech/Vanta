#include "vapch.hpp"
#include "RenderPass.hpp"

#include "Renderer.hpp"

#include "Renderer/Backend/OpenGL/OpenGLRenderPass.hpp"
#include "Renderer/Backend/Vulkan/VulkanRenderPass.hpp"

#include "Renderer/RendererAPI.hpp"

namespace Vanta {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    VA_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderPass>::Create(spec);
			case RendererAPIType::OpenGL:  return Ref<OpenGLRenderPass>::Create(spec);
		}

		VA_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}