#include "vapch.hpp"
#include "Renderer.hpp"

#include "Shader.hpp"

#include <glad/glad.h>
#include <map>

#include "RendererAPI.hpp"
#include "SceneRenderer.hpp"
#include "Renderer2D.hpp"

#include "Core/Timer.hpp"
#include "Debug/Profiler.hpp"

#include "Renderer/Backend/OpenGL/OpenGLRenderer.hpp"
#include "Renderer/Backend/Vulkan/VulkanRenderer.hpp"

#include "Renderer/Backend/Vulkan/VulkanContext.hpp"

#include "Project/Project.hpp"

#include <filesystem>

namespace Vanta {

	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<Material>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline)
	{
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}
	
	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material)
	{
		s_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void Renderer::OnShaderReloaded(size_t hash)
	{
		if (s_ShaderDependencies.find(hash) != s_ShaderDependencies.end())
		{
			auto& dependencies = s_ShaderDependencies.at(hash);
			for (auto& pipeline : dependencies.Pipelines)
			{
				pipeline->Invalidate();
			}

			for (auto& material : dependencies.Materials)
			{
				material->Invalidate();
			}
		}
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return Application::Get().GetWindow().GetSwapChain().GetCurrentBufferIndex();
	}

	void RendererAPI::SetAPI(RendererAPIType api)
	{	
		// TODO: make sure this is called at a valid time
		VA_CORE_VERIFY(api == RendererAPIType::Vulkan, "Vulkan is currently the only supported Renderer API");
		s_CurrentRendererAPI = api;
	}

	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BRDFLutTexture;
		Ref<TextureCube> BlackCubeTexture;
		Ref<Environment> EmptyEnvironment;
	};

	static RendererData* s_Data = nullptr;
	static RenderCommandQueue* s_CommandQueue = nullptr;
	static RenderCommandQueue s_ResourceFreeQueue[3];

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::OpenGL: return new OpenGLRenderer();
			case RendererAPIType::Vulkan: return new VulkanRenderer();
		}
		VA_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void Renderer::Init()
	{
		s_Data = new RendererData();
		s_CommandQueue = new RenderCommandQueue();

		//Renderer::GetConfig().FramesInFlight = 1;

		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// Compute shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentMipFilter.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/PreethamSky.glsl");

		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Grid.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SceneComposite.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/VantaPBR_Static.glsl");
		//Renderer::GetShaderLibrary()->LoadResources/Shaders("assets/shaders/VantaPBR_Anim.glsl");
		//Renderer::GetShaderLibrary()->Load("assets/shaders/Outline.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Wireframe.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/ShadowMap.glsl");

		// Renderer2D Shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Line.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/Renderer2D_Circle.glsl");

		// Jump Flood Shaders
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Init.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Pass.glsl");
		Renderer::GetShaderLibrary()->Load("Resources/Shaders/JumpFlood_Composite.glsl");

		Renderer::GetShaderLibrary()->Load("Resources/Shaders/SelectedGeometry.glsl");

		// Compile shaders
		Renderer::WaitAndRender();

		uint32_t whiteTextureData = 0xffffffff;
		s_Data->WhiteTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &whiteTextureData);

		{
			TextureProperties props;
			props.SamplerWrap = TextureWrap::Clamp;
			s_Data->BRDFLutTexture = Texture2D::Create("Resources/Renderer/BRDF_LUT.tga", props);
		}

		uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = TextureCube::Create(ImageFormat::RGBA, 1, 1, &blackTextureData);

		s_Data->EmptyEnvironment = Ref<Environment>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		s_RendererAPI->Init();
		Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();

		s_ShaderDependencies.clear();
		s_RendererAPI->Shutdown();

		delete s_Data;
		delete s_CommandQueue;
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	void Renderer::WaitAndRender()
	{
		VA_PROFILE_FUNC();
		VA_SCOPE_PERF("Renderer::WaitAndRender");
		s_CommandQueue->Execute();
	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		VA_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		s_RendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndRenderPass(renderCommandBuffer);
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow)
	{
		s_RendererAPI->SetSceneEnvironment(sceneRenderer, environment, shadow);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	Ref<TextureCube> Renderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		return s_RendererAPI->CreatePreethamSky(turbidity, azimuth, inclination);
	}

	void Renderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMesh(renderCommandBuffer, pipeline, uniformBufferSet, mesh, transform);
	}

	void Renderer::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> material, Buffer additionalUniforms)
	{
		s_RendererAPI->RenderMeshWithMaterial(renderCommandBuffer, pipeline, uniformBufferSet, mesh, material, transform, additionalUniforms);
	}

	void Renderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(renderCommandBuffer, pipeline, uniformBufferSet, material, transform);
	}

	void Renderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount /*= 0*/)
	{
		s_RendererAPI->RenderGeometry(renderCommandBuffer, pipeline, uniformBufferSet, material, vertexBuffer, indexBuffer, transform, indexCount);
	}

	void Renderer::SubmitQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, const glm::mat4& transform)
	{
		/*bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);

			auto shader = material->GetShader();
			shader->SetUniformBuffer("Transform", &transform, sizeof(glm::mat4));
		}

		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		s_Data->m_FullscreenQuadPipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);*/
	}

	void Renderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
	{
		s_RendererAPI->SubmitFullscreenQuad(renderCommandBuffer, pipeline, uniformBufferSet, material);
	}

	void Renderer::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
	{
		s_RendererAPI->SubmitFullscreenQuadWithOverrides(renderCommandBuffer, pipeline, uniformBufferSet, material, vertexShaderOverrides, fragmentShaderOverrides);
	}

#if 0
	void Renderer::SubmitFullscreenQuad(Ref<Material> material)
	{
		// Retrieve pipeline from cache
		auto& shader = material->GetShader();
		auto hash = shader->GetHash();
		if (s_PipelineCache.find(hash) == s_PipelineCache.end())
		{
			// Create pipeline
			PipelineSpecification spec = s_Data->m_FullscreenQuadPipelineSpec;
			spec.Shader = shader;
			spec.DebugName = "Renderer-FullscreenQuad-" + shader->GetName();
			s_PipelineCache[hash] = Pipeline::Create(spec);
		}

		auto& pipeline = s_PipelineCache[hash];

		bool depthTest = true;
		bool cullFace = true;
		if (material)
		{
			// material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);
		}

		s_Data->FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}
#endif

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<Texture2D> Renderer::GetBRDFLutTexture()
	{
		return s_Data->BRDFLutTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}


	Ref<Environment> Renderer::GetEmptyEnvironment()
	{
		return s_Data->EmptyEnvironment;
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue;
	}

	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}

	RendererConfig& Renderer::GetConfig()
	{
		static RendererConfig config;
		return config;
	}

}