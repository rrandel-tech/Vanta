#pragma once

#include "Renderer/UniformBuffer.hpp"

namespace Vanta {

    class OpenGLUniformBuffer : public UniformBuffer
    {
    public:
        OpenGLUniformBuffer(uint32_t size, uint32_t binding);
        virtual ~OpenGLUniformBuffer();

        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
        virtual uint32_t GetBinding() const override { return m_Binding; }
    private:
        uint32_t m_RendererID = 0;
        uint32_t m_Size = 0;
        uint32_t m_Binding = 0;
        uint8_t* m_LocalStorage = nullptr;
    };
}