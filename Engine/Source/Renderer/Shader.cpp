#include "vapch.hpp"
#include "Shader.hpp"

#include "Renderer/Backend/OpenGL/OpenGLShader.hpp"

namespace Vanta {

    Shader* Shader::Create(const std::string& filepath)
    {
        switch (RendererAPI::Current())
        {
            case RendererAPIType::None: return nullptr;
            case RendererAPIType::OpenGL: return new OpenGLShader(filepath);
        }
        return nullptr;
    }

}