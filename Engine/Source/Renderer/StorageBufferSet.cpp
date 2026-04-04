#include "vapch.hpp"

#include "UniformBufferSet.hpp"

#include "Renderer/Renderer.hpp"

#include "StorageBufferSet.hpp"

#include "Renderer/Backend/Vulkan/VulkanStorageBufferSet.hpp"
#include "Renderer/RendererAPI.hpp"

namespace Vanta {

	Ref<StorageBufferSet> StorageBufferSet::Create(uint32_t frames)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanStorageBufferSet>::Create(frames);
		}

		VA_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}