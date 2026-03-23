#include "vapch.hpp"
#include "OpenGLBuffer.hpp"

#include <glad/glad.h>

namespace Vanta {

    //////////////////////////////////////////////////////////////////////////////////
    // VertexBuffer
    //////////////////////////////////////////////////////////////////////////////////

    OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
        : m_rendererID(0), m_size(size)
    {
        VA_RENDER_S({
            glGenBuffers(1, &self->m_rendererID);
        });
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        VA_RENDER_S({
            glDeleteBuffers(1, &self->m_rendererID);
        });
    }

    void OpenGLVertexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
    {
        VA_RENDER_S3(buffer, size, offset, {
            glBindBuffer(GL_ARRAY_BUFFER, self->m_rendererID);
            glBufferData(GL_ARRAY_BUFFER, size, buffer, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
        });

    }

    void OpenGLVertexBuffer::Bind() const
    {
        VA_RENDER_S({
            glBindBuffer(GL_ARRAY_BUFFER, self->m_rendererID);
        });
    }

    //////////////////////////////////////////////////////////////////////////////////
    // IndexBuffer
    //////////////////////////////////////////////////////////////////////////////////

    OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size)
        : m_rendererID(0), m_size(size)
    {
        VA_RENDER_S({
            glGenBuffers(1, &self->m_rendererID);
            });
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        VA_RENDER_S({
            glDeleteBuffers(1, &self->m_rendererID);
            });
    }

    void OpenGLIndexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
    {
        VA_RENDER_S3(buffer, size, offset, {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->m_rendererID);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, buffer, GL_STATIC_DRAW);
            });
    }

    void OpenGLIndexBuffer::Bind() const
    {
        VA_RENDER_S({
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->m_rendererID);
            });
    }

}