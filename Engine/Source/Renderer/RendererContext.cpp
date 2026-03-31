#include "vapch.hpp"
#include "RendererContext.hpp"

#include "Renderer/RendererAPI.hpp"

#include "Renderer/Backend/OpenGL/OpenGLContext.hpp"
#include "Renderer/Backend/Vulkan/VulkanContext.hpp"

namespace Vanta {

	Ref<RendererContext> RendererContext::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::OpenGL:  return Ref<OpenGLContext>::Create();
			case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create();
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}