#include "vapch.hpp"
#include "VulkanStorageBuffer.hpp"

#include "VulkanContext.hpp"

namespace Vanta {

	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{

		Ref<VulkanStorageBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->RT_Invalidate();
		});
	}

	void VulkanStorageBuffer::RT_Invalidate()
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferInfo.size = m_Size;

		VulkanAllocator allocator("StorageBuffer");
		m_MemoryAlloc = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_Buffer);

		m_DescriptorInfo.buffer = m_Buffer;
		m_DescriptorInfo.offset = 0;
		m_DescriptorInfo.range = m_Size;
	}
	
	void VulkanStorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		memcpy(m_LocalStorage, data, size);
		Ref<VulkanStorageBuffer> instance = this;
		Renderer::Submit([instance, size, offset]() mutable
		{
			instance->RT_SetData(instance->m_LocalStorage, size, offset);
		});
	}

	void VulkanStorageBuffer::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
		VulkanAllocator allocator("VulkanStorageBuffer");
		uint8_t* pData = allocator.MapMemory<uint8_t>(m_MemoryAlloc);
		memcpy(pData, (uint8_t*)data + offset, size);
		allocator.UnmapMemory(m_MemoryAlloc);
	}

	void VulkanStorageBuffer::Resize(uint32_t newSize)
	{
		m_Size = newSize;
		Ref<VulkanStorageBuffer> instance = this;
		Renderer::Submit([instance]() mutable
			{
				instance->RT_Invalidate();
			});
	}
}
