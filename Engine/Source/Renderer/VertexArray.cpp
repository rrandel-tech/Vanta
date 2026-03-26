#include "vapch.hpp"
#include "VertexArray.hpp"

#include "Renderer.hpp"
#include "Renderer/Backend/OpenGL/OpenGLVertexArray.hpp"

namespace Vanta {

	Ref<VertexArray> VertexArray::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    VA_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPIType::OpenGL:  return std::make_shared<OpenGLVertexArray>();
		}

		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}