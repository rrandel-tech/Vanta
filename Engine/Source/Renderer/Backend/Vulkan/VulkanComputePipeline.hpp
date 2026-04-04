#pragma once

#include "Renderer/PipelineCompute.hpp"

#include "VulkanShader.hpp"
#include "VulkanTexture.hpp"
#include "VulkanRenderCommandBuffer.hpp"

#include "vulkan/vulkan.h"

namespace Vanta {

	class VulkanComputePipeline : public PipelineCompute
	{
	public:
		VulkanComputePipeline(Ref<Shader> computeShader);

		void Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		virtual void Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr) override;
		void Dispatch(VkDescriptorSet descriptorSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		virtual void End() override;

		virtual Ref<Shader> GetShader() override { return m_Shader; }

		void SetPushConstants(const void* data, uint32_t size);
		void CreatePipeline();
	private:
		void RT_CreatePipeline();
	private:
		Ref<VulkanShader> m_Shader;

		VkPipelineLayout m_ComputePipelineLayout = nullptr;
		VkPipelineCache m_PipelineCache = nullptr;
		VkPipeline m_ComputePipeline = nullptr;

		VkCommandBuffer m_ActiveComputeCommandBuffer = nullptr;

		bool m_UsingGraphicsQueue = false;
	};

}