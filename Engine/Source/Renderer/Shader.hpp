#pragma once

#include "Core/Base.hpp"
#include "Renderer/Renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

namespace Vanta
{
    struct ShaderUniform
    {
    };

    struct ShaderUniformCollection
    {
    };

    enum class UniformType
    {
        None = 0,
        Float, Float2, Float3, Float4,
        Matrix3x3, Matrix4x4,
        Int32, Uint32
    };

    struct UniformDecl
    {
        UniformType type;
        std::ptrdiff_t offset;
        std::string name;
    };

    struct UniformBuffer
    {
        // TODO: This currently represents a byte buffer that has been
        // packed with uniforms. This was primarily created for OpenGL,
        // and needs to be revisted for other rendering APIs. Furthermore,
        // this currently does not assume any alignment. This also has
        // nothing to do with GL uniform buffers, this is simply a CPU-side
        // buffer abstraction.
        byte* buffer;
        std::vector<UniformDecl> uniforms;
    };

    struct UniformBufferBase
    {
        virtual const byte* GetBuffer() const = 0;
        virtual const UniformDecl* GetUniforms() const = 0;
        virtual uint32_t GetUniformCount() const = 0;
    };

    template<uint32_t N, uint32_t U>
    struct UniformBufferDeclaration : public UniformBufferBase
    {
        byte buffer[N];
        UniformDecl uniforms[U];
        std::ptrdiff_t cursor = 0;

        virtual const byte* GetBuffer() const override { return buffer; }
        virtual const UniformDecl* GetUniforms() const override { return uniforms; }
        virtual uint32_t GetUniformCount() const { return U; }

        template<typename T>
        void Push(const std::string& name, const T& data) {}

        template<>
        void Push(const std::string& name, const float& data)
        {
            uniforms[0] = { UniformType::Float, cursor, name };
            memcpy(buffer + cursor, &data, sizeof(float));
            cursor += sizeof(float);
        }

        template<>
        void Push(const std::string& name, const glm::vec4& data)
        {
            uniforms[0] = { UniformType::Float4, cursor, name };
            memcpy(buffer + cursor, glm::value_ptr(data), sizeof(glm::vec4));
            cursor += sizeof(glm::vec4);
        }

    };

    class Shader
    {
    public:
        virtual void Bind() = 0;

        virtual void UploadUniformBuffer(const UniformBufferBase& uniformBuffer) = 0;

        // Represents a complete shader program stored in a single file.
        // Note: currently for simplicity this is simply a string filepath, however
        //       in the future this will be an asset object + metadata
        static Shader* Create(const std::string& filepath);
    };

}