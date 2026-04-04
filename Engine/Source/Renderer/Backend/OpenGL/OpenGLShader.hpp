#pragma once

#include "Renderer/Shader.hpp"
#include <glad/glad.h>
#include <spirv_cross/spirv_glsl.hpp>

namespace Vanta {

	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader() = default;
		OpenGLShader(const std::string& filepath, bool forceRecompile);
		static Ref<OpenGLShader> CreateFromString(const std::string& source);

		virtual void Reload(bool forceCompile = false) override;
		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

		virtual size_t GetHash() const override;


		RendererID GetRendererID() const { return m_RendererID; }

		void Bind();

		static const ShaderUniformBuffer& FindUniformBuffer(const std::string& name);
		static void SetUniformBuffer(const ShaderUniformBuffer& buffer, const void* data, uint32_t size, uint32_t offset = 0);
		static void SetStorageBuffer(const ShaderStorageBuffer& buffer, const void* data, uint32_t size, uint32_t offset = 0);

		virtual void SetUniformBuffer(const std::string& name, const void* data, uint32_t size);
		virtual void SetStorageBuffer(const std::string& name, const void* data, uint32_t size);

		virtual void ResizeStorageBuffer(uint32_t bindingPoint, uint32_t newSize);

		virtual void SetUniform(const std::string& fullname, float value);
		virtual void SetUniform(const std::string& fullname, int value);
		virtual void SetUniform(const std::string& fullname, const glm::ivec2& value);
		virtual void SetUniform(const std::string& fullname, const glm::ivec3& value);
		virtual void SetUniform(const std::string& fullname, const glm::ivec4& value);
		virtual void SetUniform(const std::string& fullname, uint32_t value);
		virtual void SetUniform(const std::string& fullname, const glm::vec2& value);
		virtual void SetUniform(const std::string& fullname, const glm::vec3& value);
		virtual void SetUniform(const std::string& fullname, const glm::vec4& value);
		virtual void SetUniform(const std::string& fullname, const glm::mat3& value);
		virtual void SetUniform(const std::string& fullname, const glm::mat4& value);



		virtual const std::string& GetName() const override { return m_Name; }

		virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const override { return m_Buffers; }
		virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const override { return m_Resources; }

		const ShaderResourceDeclaration* GetShaderResource(const std::string& name);

		static void ClearUniformBuffers();
	private:
		void Load(const std::string& source, bool forceCompile);
		void Compile(const std::vector<uint32_t>& vertexBinary, const std::vector<uint32_t>& fragmentBinary);
		void Reflect(std::vector<uint32_t>& data);


		void CompileOrGetVulkanBinary(std::unordered_map<uint32_t, std::vector<uint32_t>>& outputBinary, bool forceCompile = false);
		void CompileOrGetOpenGLBinary(const std::unordered_map<uint32_t, std::vector<uint32_t>>&, bool forceCompile = false);

		std::string ReadShaderFromFile(const std::string& filepath) const;
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);

		void ParseConstantBuffers(const spirv_cross::CompilerGLSL& compiler);

		int32_t GetUniformLocation(const std::string& name) const;

		static GLenum ShaderTypeFromString(const std::string& type);

		void UploadUniformInt(uint32_t location, int32_t value);
		void UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count);
		void UploadUniformFloat(uint32_t location, float value);
		void UploadUniformFloat2(uint32_t location, const glm::vec2& value);
		void UploadUniformFloat3(uint32_t location, const glm::vec3& value);
		void UploadUniformFloat4(uint32_t location, const glm::vec4& value);
		void UploadUniformMat3(uint32_t location, const glm::mat3& values);
		void UploadUniformMat4(uint32_t location, const glm::mat4& values);
		void UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count);

		void UploadUniformInt(const std::string& name, int32_t value);
		void UploadUniformUInt(const std::string& name, uint32_t value);
		void UploadUniformIntArray(const std::string& name, int32_t* values, uint32_t count);

		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value);

		void UploadUniformMat4(const std::string& name, const glm::mat4& value);
	private:
		RendererID m_RendererID = 0;
		bool m_Loaded = false;
		bool m_IsCompute = false;

		uint32_t m_ConstantBufferOffset = 0;

		std::string m_Name, m_AssetPath;
		std::unordered_map<GLenum, std::string> m_ShaderSource;

		std::vector<ShaderReloadedCallback> m_ShaderReloadedCallbacks;
		inline static std::unordered_map<uint32_t, ShaderUniformBuffer> s_UniformBuffers;
		inline static std::unordered_map<uint32_t, ShaderStorageBuffer> s_StorageBuffers;

		std::unordered_map<std::string, ShaderBuffer> m_Buffers;
		std::unordered_map<std::string, ShaderResourceDeclaration> m_Resources;
		std::unordered_map<std::string, GLint> m_UniformLocations;

	};

}