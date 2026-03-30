#include "vapch.hpp"
#include "Image.hpp"

#include "Renderer/Backend/Vulkan/VulkanImage.hpp"
#include "Renderer/Backend/OpenGL/OpenGLImage.hpp"

#include "Renderer/RendererAPI.hpp"

namespace Vanta {

	Ref<Image2D> Image2D::Create(ImageFormat format, uint32_t width, uint32_t height, Buffer buffer)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: return Ref<OpenGLImage2D>::Create(format, width, height, buffer);
			case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(format, width, height);
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: return Ref<OpenGLImage2D>::Create(format, width, height, data);
			case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(format, width, height);
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}