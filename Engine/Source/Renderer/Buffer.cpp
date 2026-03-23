#include "vapch.hpp"

#include "Renderer/Backend/OpenGL/OpenGLBuffer.hpp"

namespace Vanta {

    VertexBuffer* VertexBuffer::Create(uint32_t size)
    {
        switch (RendererAPI::Current())
        {
            case RendererAPIType::None:    return nullptr;
            case RendererAPIType::OpenGL:  return new OpenGLVertexBuffer(size);
        }
        return nullptr;
    }

    IndexBuffer* IndexBuffer::Create(uint32_t size)
    {
        switch (RendererAPI::Current())
        {
            case RendererAPIType::None:    return nullptr;
            case RendererAPIType::OpenGL:  return new OpenGLIndexBuffer(size);
        }
        return nullptr;

    }
}