#include "vapch.hpp"
#include "Pipeline.hpp"

#include "Renderer.hpp"

#include "Renderer/Backend/OpenGL/OpenGLPipeline.hpp"
#include "Renderer/Backend/Vulkan/VulkanPipeline.hpp"

#include "Renderer/RendererAPI.hpp"

namespace Vanta {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::OpenGL:  return Ref<OpenGLPipeline>::Create(spec);
			case RendererAPIType::Vulkan:  return Ref<VulkanPipeline>::Create(spec);
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}