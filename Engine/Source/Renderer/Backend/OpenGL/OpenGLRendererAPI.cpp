#include "vapch.hpp"
#include "Renderer/RendererAPI.hpp"

#include <glad/glad.h>

namespace Vanta {

    void RendererAPI::Clear(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RendererAPI::SetClearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

}