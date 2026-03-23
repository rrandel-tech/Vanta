#pragma once

#include "Renderer/Buffer.hpp"

namespace Vanta {

    //////////////////////////////////////////////////////////////////////////////////
    // VertexBuffer
    //////////////////////////////////////////////////////////////////////////////////

    class OpenGLVertexBuffer : public VertexBuffer
    {
    public:
        OpenGLVertexBuffer(uint32_t size);
        virtual ~OpenGLVertexBuffer();

        virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0);
        virtual void Bind() const;

        virtual uint32_t GetSize() const { return m_size; }
        virtual RendererID GetRendererID() const { return m_rendererID; }
    private:
        RendererID m_rendererID;
        uint32_t m_size;
    };

    //////////////////////////////////////////////////////////////////////////////////
    // IndexBuffer
    //////////////////////////////////////////////////////////////////////////////////

    class OpenGLIndexBuffer : public IndexBuffer
    {
    public:
        OpenGLIndexBuffer(uint32_t size);
        virtual ~OpenGLIndexBuffer();

        virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0);
        virtual void Bind() const;

        virtual uint32_t GetSize() const { return m_size; }
        virtual RendererID GetRendererID() const { return m_rendererID; }
    private:
        RendererID m_rendererID;
        uint32_t m_size;
    };

}