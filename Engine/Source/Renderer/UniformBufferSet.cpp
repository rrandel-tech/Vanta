#include "vapch.hpp"
#include "UniformBufferSet.hpp"

#include "Renderer/Renderer.hpp"

#include "Renderer/Backend/OpenGL/OpenGLUniformBufferSet.hpp"
#include "Renderer/Backend/Vulkan/VulkanUniformBufferSet.hpp"

#include "Renderer/RendererAPI.hpp"

namespace Vanta {

	Ref<UniformBufferSet> UniformBufferSet::Create(uint32_t frames)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanUniformBufferSet>::Create(frames);
			case RendererAPIType::OpenGL:  return Ref<OpenGLUniformBufferSet>::Create(frames);
		}

		VA_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}