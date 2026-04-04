#include "vapch.hpp"
#include "StorageBuffer.hpp"

#include "Renderer/Backend/Vulkan/VulkanStorageBuffer.hpp"
#include "Renderer/RendererAPI.hpp"

namespace Vanta {

	Ref<StorageBuffer> StorageBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:     return nullptr;
			case RendererAPIType::Vulkan:  return Ref<VulkanStorageBuffer>::Create(size, binding);
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
