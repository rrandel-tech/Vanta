#include "vapch.hpp"
#include "RendererContext.hpp"

#include "Renderer/RendererAPI.hpp"

#include "Renderer/Backend/OpenGL/OpenGLContext.hpp"
#include "Renderer/Backend/Vulkan/VulkanContext.hpp"

namespace Vanta {

	Ref<RendererContext> RendererContext::Create(SDL_Window* windowHandle)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::OpenGL:  return Ref<OpenGLContext>::Create(windowHandle);
			case RendererAPIType::Vulkan:  return Ref<VulkanContext>::Create(windowHandle);
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}