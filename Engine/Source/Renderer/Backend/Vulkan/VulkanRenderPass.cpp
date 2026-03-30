#include "vapch.hpp"
#include "VulkanRenderPass.hpp"

namespace Vanta {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
	}

}