#pragma once

#include "Renderer/UniformBufferSet.hpp"

namespace Vanta {

    class OpenGLUniformBufferSet : public UniformBufferSet
    {
    public:
        OpenGLUniformBufferSet(uint32_t frames) {}
        virtual ~OpenGLUniformBufferSet() {}

        virtual void Create(uint32_t size, uint32_t binding) override {}

        virtual Ref<UniformBuffer> Get(uint32_t binding, uint32_t set = 0, uint32_t frame = 0) override { return nullptr; }
        virtual void Set(Ref<UniformBuffer> uniformBuffer, uint32_t set = 0, uint32_t frame = 0) override { }
    };
}