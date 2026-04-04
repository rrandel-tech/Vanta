#include "vapch.hpp"
#include "PipelineCompute.hpp"

#include "Renderer/RendererAPI.hpp"
#include "Renderer/Backend/Vulkan/VulkanComputePipeline.hpp"

namespace Vanta {

	Ref<PipelineCompute> PipelineCompute::Create(Ref<Shader> computeShader)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanComputePipeline>::Create(computeShader);
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}