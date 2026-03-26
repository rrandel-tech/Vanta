#include "vapch.hpp"
#include "RenderCommandQueue.hpp"

#define VA_RENDER_TRACE(...) VA_CORE_TRACE(__VA_ARGS__)

namespace Vanta {

	constexpr size_t kCommandBufferSize = 10 * 1024 * 1024; // 10MB buffer

	RenderCommandQueue::RenderCommandQueue()
	{
		m_CommandBuffer = new uint8_t[kCommandBufferSize];
		m_CommandBufferPtr = m_CommandBuffer;
		memset(m_CommandBuffer, 0, kCommandBufferSize);
	}

	RenderCommandQueue::~RenderCommandQueue()
	{
		delete[] m_CommandBuffer;
	}

	void* RenderCommandQueue::Allocate(RenderCommandFn function, uint32_t size)
	{
		// TODO: alignment
		*(RenderCommandFn*)m_CommandBufferPtr = function;
		m_CommandBufferPtr += sizeof(RenderCommandFn);

		*(int*)m_CommandBufferPtr = size;
		m_CommandBufferPtr += sizeof(uint32_t);

		void* memory = m_CommandBufferPtr;
		m_CommandBufferPtr += size;

		m_CommandCount++;
		return memory;
	}

	void RenderCommandQueue::Execute()
	{
		// VA_RENDER_TRACE("RenderCommandQueue::Execute -- {0} commands, {1} bytes", m_commandCount, (m_CommandBufferPtr - m_commandBuffer));

		byte* buffer = m_CommandBuffer;

		for (uint32_t i = 0; i < m_CommandCount; i++)
		{
			RenderCommandFn function = *(RenderCommandFn*)buffer;
			buffer += sizeof(RenderCommandFn);

			uint32_t size = *(uint32_t*)buffer;
			buffer += sizeof(uint32_t);
			function(buffer);
			buffer += size;
		}

		m_CommandBufferPtr = m_CommandBuffer;
		m_CommandCount = 0;
	}

}