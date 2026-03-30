#include "vapch.hpp"
#include "Material.hpp"

#include "Renderer/Backend/Vulkan/VulkanMaterial.hpp"
#include "Renderer/Backend/OpenGL/OpenGLMaterial.hpp"

#include "Renderer/RendererAPI.hpp"

namespace Vanta {

	Ref<Material> Material::Create(const Ref<Shader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::Vulkan: return Ref<VulkanMaterial>::Create(shader, name);
			case RendererAPIType::OpenGL: return Ref<OpenGLMaterial>::Create(shader, name);
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	
}