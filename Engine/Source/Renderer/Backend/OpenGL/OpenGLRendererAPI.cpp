#include "vapch.hpp"
#include "Renderer/RendererAPI.hpp"

#include <glad/glad.h>

namespace Vanta {

    void RendererAPI::Init()
    {
        uint32_t vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }

    void RendererAPI::Shutdown()
    {
    }

    void RendererAPI::Clear(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RendererAPI::SetClearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

    void RendererAPI::DrawIndexed(uint32_t count)
    {
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    }

}