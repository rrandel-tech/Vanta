#include "vapch.hpp"
#include "SceneRenderer.hpp"

#include "Renderer.hpp"
#include "SceneEnvironment.hpp"

#include <glad/glad.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer2D.hpp"
#include "UniformBuffer.hpp"

#include "ImGui/ImGui.hpp"
#include "Debug/Profiler.hpp"

#include "Renderer/Backend/Vulkan/VulkanComputePipeline.hpp"
#include "Renderer/Backend/Vulkan/VulkanMaterial.hpp"
#include "Renderer/Backend/Vulkan/VulkanRenderer.hpp"

namespace Vanta {

	static std::vector<std::thread> s_ThreadPool;

	SceneRenderer::SceneRenderer(Ref<Scene> scene, SceneRendererSpecification specification)
		: m_Scene(scene), m_Specification(specification)
	{
		Init();
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	void SceneRenderer::Init()
	{
		VA_SCOPE_TIMER("SceneRenderer::Init");

		if (m_Specification.SwapChainTarget)
			m_CommandBuffer = RenderCommandBuffer::CreateFromSwapChain("SceneRenderer");
		else
			m_CommandBuffer = RenderCommandBuffer::Create(0, "SceneRenderer");

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		m_UniformBufferSet = UniformBufferSet::Create(framesInFlight);
		m_UniformBufferSet->Create(sizeof(UBCamera), 0);
		m_UniformBufferSet->Create(sizeof(UBShadow), 1);
		m_UniformBufferSet->Create(sizeof(UBScene), 2);
		m_UniformBufferSet->Create(sizeof(UBRendererData), 3);

		m_CompositeShader = Renderer::GetShaderLibrary()->Get("SceneComposite");
		CompositeMaterial = Material::Create(m_CompositeShader);

		// Shadow pass
		{
			ImageSpecification spec;
			spec.Format = ImageFormat::DEPTH32F;
			spec.Usage = ImageUsage::Attachment;
			spec.Width = 4096;
			spec.Height = 4096;
			spec.Layers = 4; // 4 cascades
			Ref<Image2D> cascadedDepthImage = Image2D::Create(spec);
			cascadedDepthImage->Invalidate();
			cascadedDepthImage->CreatePerLayerImageViews();

			FramebufferSpecification shadowMapFramebufferSpec;
			shadowMapFramebufferSpec.Width = 4096;
			shadowMapFramebufferSpec.Height = 4096;
			shadowMapFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
			shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			shadowMapFramebufferSpec.NoResize = true;
			shadowMapFramebufferSpec.ExistingImage = cascadedDepthImage;
			shadowMapFramebufferSpec.DebugName = "Shadow Map";

			// 4 cascades
			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("ShadowMap");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "ShadowPass";
			pipelineSpec.Shader = shadowPassShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			for (int i = 0; i < 4; i++)
			{
				shadowMapFramebufferSpec.ExistingImageLayer = i;

				RenderPassSpecification shadowMapRenderPassSpec;
				shadowMapRenderPassSpec.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);
				shadowMapRenderPassSpec.DebugName = "ShadowMap";
				ShadowMapRenderPass[i] = RenderPass::Create(shadowMapRenderPassSpec);

				pipelineSpec.RenderPass = ShadowMapRenderPass[i];
				m_ShadowPassPipelines[i] = Pipeline::Create(pipelineSpec);
				m_ShadowPassMaterial = Material::Create(shadowPassShader, "ShadowPass");
			}
		}

		// Geometry
		{
			FramebufferSpecification geoFramebufferSpec;
			geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
			geoFramebufferSpec.Samples = 1;
			geoFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			geoFramebufferSpec.DebugName = "Geometry";

			Ref<Framebuffer> framebuffer = Framebuffer::Create(geoFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("VantaPBR_Static");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "Geometry";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "PBR-Static";
			m_GeometryPipeline = Pipeline::Create(pipelineSpecification);

			pipelineSpecification.Wireframe = true;
			pipelineSpecification.DepthTest = false;
			pipelineSpecification.LineWidth = 2.0f;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Wireframe");
			pipelineSpecification.DebugName = "Wireframe";
			m_GeometryWireframePipeline = Pipeline::Create(pipelineSpecification);

		}

		// Selected Geometry isolation (for outline pass)
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "SelectedGeometry";
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};

			FramebufferSpecification framebufferSpec;
			framebufferSpec.DebugName = pipelineSpecification.DebugName;
			framebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
			framebufferSpec.Samples = 1;
			framebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

			RenderPassSpecification renderPassSpec;
			renderPassSpec.DebugName = pipelineSpecification.DebugName;
			renderPassSpec.TargetFramebuffer = Framebuffer::Create(framebufferSpec);
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry");
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			m_SelectedGeometryPipeline = Pipeline::Create(pipelineSpecification);

			m_SelectedGeometryMaterial = Material::Create(pipelineSpecification.Shader);
		}

		// Bloom Compute
		{
			auto shader = Renderer::GetShaderLibrary()->Get("Bloom");
			m_BloomComputePipeline = PipelineCompute::Create(shader);
			TextureProperties props;
			props.SamplerWrap = TextureWrap::Clamp;
			m_BloomComputeTextures[0] = Texture2D::Create(ImageFormat::RGBA32F, 1, 1, nullptr, props);
			m_BloomComputeTextures[1] = Texture2D::Create(ImageFormat::RGBA32F, 1, 1, nullptr, props);
			m_BloomComputeTextures[2] = Texture2D::Create(ImageFormat::RGBA32F, 1, 1, nullptr, props);
			m_BloomComputeMaterial = Material::Create(shader);
			
			m_BloomDirtTexture = Renderer::GetBlackTexture();
		}

		// Composite
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.DebugName = "SceneComposite";
			compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			compFramebufferSpec.SwapChainTarget = m_Specification.SwapChainTarget;
			// No depth for swapchain
			if (m_Specification.SwapChainTarget)
				compFramebufferSpec.Attachments = { ImageFormat::RGBA };
			else
				compFramebufferSpec.Attachments = { ImageFormat::RGBA, ImageFormat::Depth };

			Ref<Framebuffer> framebuffer = Framebuffer::Create(compFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SceneComposite");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "SceneComposite";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "SceneComposite";
			pipelineSpecification.DepthWrite = false;
			m_CompositePipeline = Pipeline::Create(pipelineSpecification);
		}

		// External compositing
		if (!m_Specification.SwapChainTarget)
		{
			FramebufferSpecification extCompFramebufferSpec;
			extCompFramebufferSpec.Attachments = { ImageFormat::RGBA, ImageFormat::Depth };
			extCompFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			extCompFramebufferSpec.ClearOnLoad = false;
			extCompFramebufferSpec.DebugName = "External Composite";
			
			// Use the color buffer from the final compositing pass, but the depth buffer from
			// the actual 3D geometry pass, in case we want to composite elements behind meshes
			// in the scene
			extCompFramebufferSpec.ExistingImages[0] = m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage();
			extCompFramebufferSpec.ExistingImages[1] = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage();

			Ref<Framebuffer> framebuffer = Framebuffer::Create(extCompFramebufferSpec);

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "External-Composite";
			m_ExternalCompositeRenderPass = RenderPass::Create(renderPassSpec);
		}

		// Temporary framebuffers for re-use
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Attachments = { ImageFormat::RGBA32F };
			framebufferSpec.Samples = 1;
			framebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			framebufferSpec.BlendMode = FramebufferBlendMode::OneZero;

			for (uint32_t i = 0; i < 2; i++)
				m_TempFramebuffers.emplace_back(Framebuffer::Create(framebufferSpec));
		}

		// Jump Flood (outline)
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Init");
			m_JumpFloodInitMaterial = Material::Create(pipelineSpecification.Shader, "JumpFlood-Init");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = m_TempFramebuffers[0];
			renderPassSpec.DebugName = "JumpFlood-Init";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "JumpFlood-Init";
			m_JumpFloodInitPipeline = Pipeline::Create(pipelineSpecification);

			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Pass");
			m_JumpFloodPassMaterial[0] = Material::Create(pipelineSpecification.Shader, "JumpFloodPass-0");
			m_JumpFloodPassMaterial[1] = Material::Create(pipelineSpecification.Shader, "JumpFloodPass-1");

			const char* passName[2] = { "EvenPass", "OddPass" };
			for (uint32_t i = 0; i < 2; i++)
			{
				renderPassSpec.TargetFramebuffer = m_TempFramebuffers[(i + 1) % 2];
				renderPassSpec.DebugName = std::format("JumpFlood-{0}", passName[i]);

				pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
				pipelineSpecification.DebugName = renderPassSpec.DebugName;

				m_JumpFloodPassPipeline[i] = Pipeline::Create(pipelineSpecification);
			}

			// Outline compositing
			{
				pipelineSpecification.RenderPass = m_CompositePipeline->GetSpecification().RenderPass;
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Composite");
				pipelineSpecification.DebugName = "JumpFlood-Composite";
				pipelineSpecification.DepthTest = false;
				m_JumpFloodCompositePipeline = Pipeline::Create(pipelineSpecification);

				m_JumpFloodCompositeMaterial = Material::Create(pipelineSpecification.Shader, "JumpFlood-Composite");
			}
		}

		// Grid
		{
			m_GridShader = Renderer::GetShaderLibrary()->Get("Grid");
			const float gridScale = 16.025f;
			const float gridSize = 0.025f;
			m_GridMaterial = Material::Create(m_GridShader);
			m_GridMaterial->Set("u_Settings.Scale", gridScale);
			m_GridMaterial->Set("u_Settings.Size", gridSize);

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = m_GridShader;
			pipelineSpec.BackfaceCulling = false;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = m_GeometryPipeline->GetSpecification().RenderPass;
			m_GridPipeline = Pipeline::Create(pipelineSpec);
		}

		m_WireframeMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Wireframe"));
		m_WireframeMaterial->Set("u_MaterialUniforms.Color", glm::vec4{ 1.0f, 0.5f, 0.0f, 1.0f });

		// Skybox
		{
			auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Skybox";
			pipelineSpec.Shader = skyboxShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = m_GeometryPipeline->GetSpecification().RenderPass;
			m_SkyboxPipeline = Pipeline::Create(pipelineSpec);

			m_SkyboxMaterial = Material::Create(skyboxShader);
			m_SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, false);
		}

		Ref<SceneRenderer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->m_ResourcesCreated = true;
		});
	}

	void SceneRenderer::SetScene(Ref<Scene> scene)
	{
		VA_CORE_ASSERT(!m_Active, "Can't change scenes while rendering");
		m_Scene = scene;
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (m_ViewportWidth != width || m_ViewportHeight != height)
		{
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			m_NeedsResize = true;
		}
	}

	void SceneRenderer::CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection) const
	{
		struct FrustumBounds
		{
			float r, l, b, t, f, n;
		};

		//FrustumBounds frustumBounds[3];

		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		// TODO: less hard-coding!
		float nearClip = 0.1f;
		float farClip = 1000.0f;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		// Manually set cascades here
		// cascadeSplits[0] = 0.05f;
		// cascadeSplits[1] = 0.15f;
		// cascadeSplits[2] = 0.3f;
		// cascadeSplits[3] = 1.0f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			const float ShadowMapResolution = 4096.0f;
			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		VA_PROFILE_FUNC();

		VA_CORE_ASSERT(m_Scene);
		VA_CORE_ASSERT(!m_Active);
		m_Active = true;

		if (!m_ResourcesCreated)
			return;

		m_SceneData.SceneCamera = camera;
		m_SceneData.SceneEnvironment = m_Scene->m_Environment;
		m_SceneData.SceneEnvironmentIntensity = m_Scene->m_EnvironmentIntensity;
		m_SceneData.ActiveLight = m_Scene->m_Light;
		m_SceneData.SceneLightEnvironment = m_Scene->m_LightEnvironment;
		m_SceneData.SkyboxLod = m_Scene->m_SkyboxLod;
		m_SceneData.ActiveLight = m_Scene->m_Light;

		if (m_NeedsResize)
		{
			m_NeedsResize = false;

			m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			
			// Half-size bloom texture
			{
				uint32_t viewportWidth = m_ViewportWidth / 2;
				uint32_t viewportHeight = m_ViewportHeight / 2;
				viewportWidth += (m_BloomComputeWorkgroupSize - (viewportWidth % m_BloomComputeWorkgroupSize));
				viewportHeight += (m_BloomComputeWorkgroupSize - (viewportHeight % m_BloomComputeWorkgroupSize));
				m_BloomComputeTextures[0]->Resize(viewportWidth, viewportHeight);
				m_BloomComputeTextures[1]->Resize(viewportWidth, viewportHeight);
				m_BloomComputeTextures[2]->Resize(viewportWidth, viewportHeight);
			}

			for (auto& tempFB : m_TempFramebuffers)
				tempFB->Resize(m_ViewportWidth, m_ViewportHeight);

			if (m_ExternalCompositeRenderPass)
				m_ExternalCompositeRenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);

			if (m_Specification.SwapChainTarget)
				m_CommandBuffer = RenderCommandBuffer::CreateFromSwapChain("SceneRenderer");
		}

		// Update uniform buffers
		UBCamera& cameraData = CameraData;
		UBScene& sceneData = SceneDataUB;
		UBShadow& shadowData = ShadowData;
		UBRendererData& rendererData = RendererDataUB;

		auto& sceneCamera = m_SceneData.SceneCamera;
		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(sceneCamera.ViewMatrix)[3];

		auto inverseVP = glm::inverse(viewProjection);
		cameraData.ViewProjection = viewProjection;
		cameraData.InverseViewProjection = inverseVP;
		cameraData.Projection = sceneCamera.Camera.GetProjectionMatrix();
		cameraData.View = sceneCamera.ViewMatrix;
		Ref<SceneRenderer> instance = this;
		Renderer::Submit([instance, cameraData]() mutable
		{
			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
			instance->m_UniformBufferSet->Get(0, 0, bufferIndex)->RT_SetData(&cameraData, sizeof(cameraData));
		});

		const auto& directionalLight = m_SceneData.SceneLightEnvironment.DirectionalLights[0];
		sceneData.lights.Direction = directionalLight.Direction;
		sceneData.lights.Radiance = directionalLight.Radiance;
		sceneData.lights.Multiplier = directionalLight.Multiplier;
		sceneData.u_CameraPosition = cameraPosition;
		sceneData.EnvironmentMapIntensity = m_SceneData.SceneEnvironmentIntensity;
		Renderer::Submit([instance, sceneData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->m_UniformBufferSet->Get(2, 0, bufferIndex)->RT_SetData(&sceneData, sizeof(sceneData));
			});

		CascadeData cascades[4];
		CalculateCascades(cascades, sceneCamera, directionalLight.Direction);

		// TODO: four cascades for now
		for (int i = 0; i < 4; i++)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			shadowData.ViewProjection[i] = cascades[i].ViewProj;
		}
		Renderer::Submit([instance, shadowData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->m_UniformBufferSet->Get(1, 0, bufferIndex)->RT_SetData(&shadowData, sizeof(shadowData));
			});

		rendererData.CascadeSplits = CascadeSplits;
		Renderer::Submit([instance, rendererData]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				instance->m_UniformBufferSet->Get(3, 0, bufferIndex)->RT_SetData(&rendererData, sizeof(rendererData));
			});

		Renderer::SetSceneEnvironment(this, m_SceneData.SceneEnvironment, m_ShadowPassPipelines[0]->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage());
	}

	void SceneRenderer::EndScene()
	{
		VA_PROFILE_FUNC();

		VA_CORE_ASSERT(m_Active);
#if MULTI_THREAD
		Ref<SceneRenderer> instance = this;
		s_ThreadPool.emplace_back(([instance]() mutable
			{
				instance->FlushDrawList();
	}));
#else 
		FlushDrawList();
#endif

		m_Active = false;
}

	void SceneRenderer::WaitForThreads()
	{
		for (uint32_t i = 0; i < s_ThreadPool.size(); i++)
			s_ThreadPool[i].join();

		s_ThreadPool.clear();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		m_DrawList.push_back({ mesh, overrideMaterial, transform });
		m_ShadowPassDrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		m_SelectedMeshDrawList.push_back({ mesh, nullptr, transform });
		m_ShadowPassDrawList.push_back({ mesh, nullptr, transform });
	}

	void SceneRenderer::ShadowMapPass()
	{
		VA_PROFILE_FUNC();

		m_GPUTimeQueries.ShadowMapPassQuery = m_CommandBuffer->BeginTimestampQuery();

		auto& directionalLights = m_SceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
		{
			for (int i = 0; i < 4; i++)
			{
				// Clear shadow maps
				Renderer::BeginRenderPass(m_CommandBuffer, ShadowMapRenderPass[i]);
				Renderer::EndRenderPass(m_CommandBuffer);
			}
			return;
		}

		// TODO: change to four cascades (or set number)
		for (int i = 0; i < 4; i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, ShadowMapRenderPass[i]);

			// static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });

			// Render entities
			Buffer cascade(&i, sizeof(uint32_t));
			for (auto& dc : m_ShadowPassDrawList)
			{
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelines[i], m_UniformBufferSet, nullptr, dc.Mesh, dc.Transform, m_ShadowPassMaterial, cascade);
			}

			Renderer::EndRenderPass(m_CommandBuffer);
		}

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.ShadowMapPassQuery);
	}

	void SceneRenderer::GeometryPass()
	{
		VA_PROFILE_FUNC();

		m_GPUTimeQueries.GeometryPassQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::BeginRenderPass(m_CommandBuffer, m_SelectedGeometryPipeline->GetSpecification().RenderPass);
		for (auto& dc : m_SelectedMeshDrawList)
		{
			Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SelectedGeometryPipeline, m_UniformBufferSet, nullptr, dc.Mesh, dc.Transform, m_SelectedGeometryMaterial);
		}
		Renderer::EndRenderPass(m_CommandBuffer);

		Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryPipeline->GetSpecification().RenderPass);
		// Skybox
		m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SceneData.SkyboxLod);
		m_SkyboxMaterial->Set("u_Uniforms.Intensity", m_SceneData.SceneEnvironmentIntensity);

		Ref<TextureCube> radianceMap = m_SceneData.SceneEnvironment ? m_SceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		m_SkyboxMaterial->Set("u_Texture", radianceMap);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_SkyboxPipeline, m_UniformBufferSet, nullptr, m_SkyboxMaterial);

		// Render entities
		for (auto& dc : m_DrawList)
			Renderer::RenderMesh(m_CommandBuffer, m_GeometryPipeline, m_UniformBufferSet, m_StorageBufferSet, dc.Mesh, dc.Transform);

		for (auto& dc : m_SelectedMeshDrawList)
		{
			Renderer::RenderMesh(m_CommandBuffer, m_GeometryPipeline, m_UniformBufferSet, m_StorageBufferSet, dc.Mesh, dc.Transform);
			if (m_Options.ShowSelectedInWireframe)
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_GeometryWireframePipeline, m_UniformBufferSet, nullptr, dc.Mesh, dc.Transform, m_WireframeMaterial);
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(8.0f));
			Renderer::RenderQuad(m_CommandBuffer, m_GridPipeline, m_UniformBufferSet, nullptr, m_GridMaterial, transform);
		}

		Renderer::EndRenderPass(m_CommandBuffer);

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.GeometryPassQuery);
	}

	void SceneRenderer::JumpFloodPass()
	{
		VA_PROFILE_FUNC();

		m_GPUTimeQueries.JumpFloodPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodInitPipeline->GetSpecification().RenderPass);

		auto framebuffer = m_SelectedGeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
		m_JumpFloodInitMaterial->Set("u_Texture", framebuffer->GetImage());

		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodInitPipeline, nullptr, m_JumpFloodInitMaterial);
		Renderer::EndRenderPass(m_CommandBuffer);

		m_JumpFloodPassMaterial[0]->Set("u_Texture", m_TempFramebuffers[0]->GetImage());
		m_JumpFloodPassMaterial[1]->Set("u_Texture", m_TempFramebuffers[1]->GetImage());

		int steps = 2;
		int step = std::round(std::pow(steps - 1, 2));
		int index = 0;
		Buffer vertexOverrides;
		Ref<Framebuffer> passFB = m_JumpFloodPassPipeline[0]->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
		glm::vec2 texelSize = { 1.0f / (float)passFB->GetWidth(), 1.0f / (float)passFB->GetHeight() };
		vertexOverrides.Allocate(sizeof(glm::vec2) + sizeof(int));
		vertexOverrides.Write(glm::value_ptr(texelSize), sizeof(glm::vec2));
		while (step != 0)
		{
			vertexOverrides.Write(&step, sizeof(int), sizeof(glm::vec2));

			Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodPassPipeline[index]->GetSpecification().RenderPass);
			Renderer::SubmitFullscreenQuadWithOverrides(m_CommandBuffer, m_JumpFloodPassPipeline[index], nullptr, m_JumpFloodPassMaterial[index], vertexOverrides, Buffer());
			Renderer::EndRenderPass(m_CommandBuffer);
			
			index = (index + 1) % 2;
			step /= 2;
		}
		
		m_JumpFloodCompositeMaterial->Set("u_Texture", m_TempFramebuffers[1]->GetImage());
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.JumpFloodPassQuery);
	}

	void SceneRenderer::BloomCompute()
	{
		Ref<VulkanComputePipeline> pipeline = m_BloomComputePipeline.As<VulkanComputePipeline>();

		//m_BloomComputeMaterial->Set("o_Image", m_BloomComputeTexture);

		struct BloomComputePushConstants
		{
			glm::vec4 Params;
			float LOD = 0.0f;
			int Mode = 0; // 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
		} bloomComputePushConstants;
		bloomComputePushConstants.Params = { m_BloomSettings.Threshold, m_BloomSettings.Threshold - m_BloomSettings.Knee, m_BloomSettings.Knee * 2.0f, 0.25f / m_BloomSettings.Knee };
		bloomComputePushConstants.Mode = 0;

		auto inputTexture = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage().As<VulkanImage2D>();

		m_GPUTimeQueries.BloomComputePassQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::Submit([bloomComputePushConstants, inputTexture, workGroupSize = m_BloomComputeWorkgroupSize, commandBuffer = m_CommandBuffer, bloomTextures = m_BloomComputeTextures, ubs = m_UniformBufferSet, material = m_BloomComputeMaterial.As<VulkanMaterial>(), pipeline]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			Ref<VulkanImage2D> images[3] =
			{
				bloomTextures[0]->GetImage().As<VulkanImage2D>(),
				bloomTextures[1]->GetImage().As<VulkanImage2D>(),
				bloomTextures[2]->GetImage().As<VulkanImage2D>()
			};

			auto shader = material->GetShader().As<VulkanShader>();

			auto descriptorImageInfo = images[0]->GetDescriptor();
			descriptorImageInfo.imageView = images[0]->RT_GetMipImageView(0);

			std::array<VkWriteDescriptorSet, 3> writeDescriptors;

			VkDescriptorSetLayout descriptorSetLayout = shader->GetDescriptorSetLayout(0);

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;

			pipeline->Begin(commandBuffer);

			// Output image
			VkDescriptorSet descriptorSet = VulkanRenderer::RT_AllocateDescriptorSet(allocInfo);
			writeDescriptors[0] = *shader->GetDescriptorSet("o_Image");
			writeDescriptors[0].dstSet = descriptorSet; // Should this be set inside the shader?
			writeDescriptors[0].pImageInfo = &descriptorImageInfo;

			// Input image
			writeDescriptors[1] = *shader->GetDescriptorSet("u_Texture");
			writeDescriptors[1].dstSet = descriptorSet; // Should this be set inside the shader?
			writeDescriptors[1].pImageInfo = &inputTexture->GetDescriptor();

			writeDescriptors[2] = *shader->GetDescriptorSet("u_BloomTexture");
			writeDescriptors[2].dstSet = descriptorSet; // Should this be set inside the shader?
			writeDescriptors[2].pImageInfo = &inputTexture->GetDescriptor();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

			uint32_t workGroupsX = bloomTextures[0]->GetWidth() / workGroupSize;
			uint32_t workGroupsY = bloomTextures[0]->GetHeight() / workGroupSize;

			pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));
			pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);

			bloomComputePushConstants.Mode = 1;

			VkSampler samplerClamp = VulkanRenderer::GetClampSampler();

			uint32_t mips = bloomTextures[0]->GetMipLevelCount() - 2;
			for (uint32_t i = 1; i < mips; i++)
			{
				auto[mipWidth, mipHeight] = bloomTextures[0]->GetMipSize(i);
				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)workGroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)workGroupSize);

				{
					// Output image
					descriptorImageInfo.imageView = images[1]->RT_GetMipImageView(i);
					
					descriptorSet = VulkanRenderer::RT_AllocateDescriptorSet(allocInfo);
					writeDescriptors[0] = *shader->GetDescriptorSet("o_Image");
					writeDescriptors[0].dstSet = descriptorSet; // Should this be set inside the shader?
					writeDescriptors[0].pImageInfo = &descriptorImageInfo;

					// Input image
					writeDescriptors[1] = *shader->GetDescriptorSet("u_Texture");
					writeDescriptors[1].dstSet = descriptorSet; // Should this be set inside the shader?
					auto descriptor = bloomTextures[0]->GetImage().As<VulkanImage2D>()->GetDescriptor();
					//descriptor.sampler = samplerClamp;
					writeDescriptors[1].pImageInfo = &descriptor;

					writeDescriptors[2] = *shader->GetDescriptorSet("u_BloomTexture");
					writeDescriptors[2].dstSet = descriptorSet; // Should this be set inside the shader?
					writeDescriptors[2].pImageInfo = &inputTexture->GetDescriptor();

					vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

					bloomComputePushConstants.LOD = i - 1.0f;
					pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));
					pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);
				}
				{
					descriptorImageInfo.imageView = images[0]->RT_GetMipImageView(i);

					// Output image
					descriptorSet = VulkanRenderer::RT_AllocateDescriptorSet(allocInfo);
					writeDescriptors[0] = *shader->GetDescriptorSet("o_Image");
					writeDescriptors[0].dstSet = descriptorSet; // Should this be set inside the shader?
					writeDescriptors[0].pImageInfo = &descriptorImageInfo;

					// Input image
					writeDescriptors[1] = *shader->GetDescriptorSet("u_Texture");
					writeDescriptors[1].dstSet = descriptorSet; // Should this be set inside the shader?
					auto descriptor = bloomTextures[1]->GetImage().As<VulkanImage2D>()->GetDescriptor();
					//descriptor.sampler = samplerClamp;
					writeDescriptors[1].pImageInfo = &descriptor;

					writeDescriptors[2] = *shader->GetDescriptorSet("u_BloomTexture");
					writeDescriptors[2].dstSet = descriptorSet; // Should this be set inside the shader?
					writeDescriptors[2].pImageInfo = &inputTexture->GetDescriptor();

					vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

					bloomComputePushConstants.LOD = i;
					pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));
					pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);
				}
			}

			bloomComputePushConstants.Mode = 2;
			workGroupsX *= 2;
			workGroupsY *= 2;

			// Output image
			descriptorSet = VulkanRenderer::RT_AllocateDescriptorSet(allocInfo);
			descriptorImageInfo.imageView = images[2]->RT_GetMipImageView(mips - 2);

			writeDescriptors[0] = *shader->GetDescriptorSet("o_Image");
			writeDescriptors[0].dstSet = descriptorSet; // Should this be set inside the shader?
			writeDescriptors[0].pImageInfo = &descriptorImageInfo;

			// Input image
			writeDescriptors[1] = *shader->GetDescriptorSet("u_Texture");
			writeDescriptors[1].dstSet = descriptorSet; // Should this be set inside the shader?
			writeDescriptors[1].pImageInfo = &bloomTextures[0]->GetImage().As<VulkanImage2D>()->GetDescriptor();

			writeDescriptors[2] = *shader->GetDescriptorSet("u_BloomTexture");
			writeDescriptors[2].dstSet = descriptorSet; // Should this be set inside the shader?
			writeDescriptors[2].pImageInfo = &inputTexture->GetDescriptor();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

			bloomComputePushConstants.LOD--;
			pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));

			auto [mipWidth, mipHeight] = bloomTextures[2]->GetMipSize(mips - 2);
			workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)workGroupSize);
			workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)workGroupSize);
			pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);

			bloomComputePushConstants.Mode = 3;

			// Upsample
			for (int32_t mip = mips - 3; mip >= 0; mip--)
			{
				auto [mipWidth, mipHeight] = bloomTextures[2]->GetMipSize(mip);
				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)workGroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)workGroupSize);

				// Output image
				descriptorImageInfo.imageView = images[2]->RT_GetMipImageView(mip);
				auto descriptorSet = VulkanRenderer::RT_AllocateDescriptorSet(allocInfo);
				writeDescriptors[0] = *shader->GetDescriptorSet("o_Image");
				writeDescriptors[0].dstSet = descriptorSet; // Should this be set inside the shader?
				writeDescriptors[0].pImageInfo = &descriptorImageInfo;

				// Input image
				writeDescriptors[1] = *shader->GetDescriptorSet("u_Texture");
				writeDescriptors[1].dstSet = descriptorSet; // Should this be set inside the shader?
				writeDescriptors[1].pImageInfo = &bloomTextures[0]->GetImage().As<VulkanImage2D>()->GetDescriptor();

				writeDescriptors[2] = *shader->GetDescriptorSet("u_BloomTexture");
				writeDescriptors[2].dstSet = descriptorSet; // Should this be set inside the shader?
				writeDescriptors[2].pImageInfo = &images[2]->GetDescriptor();

				vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

				bloomComputePushConstants.LOD = mip;
				pipeline->SetPushConstants(&bloomComputePushConstants, sizeof(bloomComputePushConstants));
				pipeline->Dispatch(descriptorSet, workGroupsX, workGroupsY, 1);
			}

			pipeline->End();
		});
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.BloomComputePassQuery);
	}

	void SceneRenderer::CompositePass()
	{
		VA_PROFILE_FUNC();

		m_GPUTimeQueries.CompositePassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginRenderPass(m_CommandBuffer, m_CompositePipeline->GetSpecification().RenderPass, true);

		auto framebuffer = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
		float exposure = m_SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = framebuffer->GetSpecification().Samples;

		CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		if (m_BloomSettings.Enabled)
		{
			CompositeMaterial->Set("u_Uniforms.BloomIntensity", m_BloomSettings.Intensity);
			CompositeMaterial->Set("u_Uniforms.BloomDirtIntensity", m_BloomSettings.DirtIntensity);
		}
		else
		{
			CompositeMaterial->Set("u_Uniforms.BloomIntensity", 0.0f);
			CompositeMaterial->Set("u_Uniforms.BloomDirtIntensity", 0.0f);
		}

		//CompositeMaterial->Set("u_Uniforms.TextureSamples", textureSamples);

		CompositeMaterial->Set("u_Texture", framebuffer->GetImage());
		CompositeMaterial->Set("u_BloomTexture", m_BloomComputeTextures[2]);
		CompositeMaterial->Set("u_BloomDirtTexture", m_BloomDirtTexture);

		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_CompositePipeline, nullptr, CompositeMaterial);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodCompositePipeline, nullptr, m_JumpFloodCompositeMaterial);
		Renderer::EndRenderPass(m_CommandBuffer);

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.CompositePassQuery);

#if 0 // WIP
		// DOF
		Renderer::BeginRenderPass(m_CommandBuffer, m_DOFPipeline->GetSpecification().RenderPass);
		m_DOFMaterial->Set("u_Texture", m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage());
		m_DOFMaterial->Set("u_DepthTexture", m_PreDepthPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage());
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_DOFPipeline, nullptr, m_DOFMaterial);
		Renderer::EndRenderPass(m_CommandBuffer);
#endif

		//Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodCompositePipeline->GetSpecification().RenderPass);
		//Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRenderer::FlushDrawList()
	{
		if (m_ResourcesCreated)
		{
			m_CommandBuffer->Begin();

			// Main render passes
			ShadowMapPass();
			GeometryPass();

			// Post-processing
			JumpFloodPass();
			BloomCompute();
			CompositePass();

			m_CommandBuffer->End();
			m_CommandBuffer->Submit();
		}
		else
		{
			// Empty pass
			m_CommandBuffer->Begin();

			ClearPass();

			m_CommandBuffer->End();
			m_CommandBuffer->Submit();
		}

		m_DrawList.clear();
		m_SelectedMeshDrawList.clear();
		m_ShadowPassDrawList.clear();
		m_SceneData = {};
	}

	void SceneRenderer::ClearPass()
	{
		VA_PROFILE_FUNC();

		Renderer::BeginRenderPass(m_CommandBuffer, m_CompositePipeline->GetSpecification().RenderPass, true);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return m_CompositePipeline->GetSpecification().RenderPass;
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage()
	{
		if (!m_ResourcesCreated)
			return nullptr;

		return m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage();
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return m_Options;
	}

	void SceneRenderer::OnImGuiRender()
	{
		VA_PROFILE_FUNC();

		ImGui::Begin("Scene Renderer");

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Shader::s_AllShaders;
			for (auto& shader : shaders)
			{
				if (ImGui::TreeNode(shader->GetName().c_str()))
				{
					std::string buttonName = "Reload##" + shader->GetName();
					if (ImGui::Button(buttonName.c_str()))
						shader->Reload(true);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		if (UI::BeginTreeNode("Visualization"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Show Shadow Cascades", RendererDataUB.ShowCascades);
			static int maxDrawCall = 1000;
			UI::PropertySlider("Selected Draw", VulkanRenderer::GetSelectedDrawCall(), -1, maxDrawCall);
			UI::Property("Max Draw Call", maxDrawCall);
			UI::EndPropertyGrid();
			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Render Statistics"))
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			ImGui::Text("GPU time: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex));

			ImGui::Text("Shadow Map Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.ShadowMapPassQuery));
			ImGui::Text("Geometry Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.GeometryPassQuery));
			ImGui::Text("Bloom Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.BloomComputePassQuery));
			ImGui::Text("Jump Flood Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.JumpFloodPassQuery));
			ImGui::Text("Composite Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.CompositePassQuery));

			if (UI::BeginTreeNode("Pipeline Statistics"))
			{
				const PipelineStatistics& pipelineStats = m_CommandBuffer->GetPipelineStatistics(frameIndex);
				ImGui::Text("Input Assembly Vertices: %llu", pipelineStats.InputAssemblyVertices);
				ImGui::Text("Input Assembly Primitives: %llu", pipelineStats.InputAssemblyPrimitives);
				ImGui::Text("Vertex Shader Invocations: %llu", pipelineStats.VertexShaderInvocations);
				ImGui::Text("Clipping Invocations: %llu", pipelineStats.ClippingInvocations);
				ImGui::Text("Clipping Primitives: %llu", pipelineStats.ClippingPrimitives);
				ImGui::Text("Fragment Shader Invocations: %llu", pipelineStats.FragmentShaderInvocations);
				ImGui::Text("Compute Shader Invocations: %llu", pipelineStats.ComputeShaderInvocations);
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Bloom Settings"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Bloom Enabled", m_BloomSettings.Enabled);
			UI::Property("Threshold", m_BloomSettings.Threshold);
			UI::Property("Knee", m_BloomSettings.Knee);
			UI::Property("Upsample Scale", m_BloomSettings.UpsampleScale);
			UI::Property("Intensity", m_BloomSettings.Intensity, 0.05f, 0.0f, 20.0f);
			UI::Property("Dirt Intensity", m_BloomSettings.DirtIntensity, 0.05f, 0.0f, 20.0f);

			// TODO: move this to somewhere else
			UI::Image(m_BloomDirtTexture, ImVec2(64, 64));
			if (ImGui::IsItemHovered())
			{
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get().OpenFile("");
					if (!filename.empty())
						m_BloomDirtTexture = Texture2D::Create(filename);
				}
			}

			UI::EndPropertyGrid();
			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Shadows"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Soft Shadows", RendererDataUB.SoftShadows);
			UI::Property("DirLight Size", RendererDataUB.LightSize, 0.01f);
			UI::Property("Max Shadow Distance", RendererDataUB.MaxShadowDistance, 1.0f);
			UI::Property("Shadow Fade", RendererDataUB.ShadowFade, 5.0f);
			UI::EndPropertyGrid();
			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid();
				UI::Property("Cascade Fading", RendererDataUB.CascadeFading);
				UI::Property("Cascade Transition Fade", RendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
				UI::Property("Cascade Split", CascadeSplitLambda, 0.01f);
				UI::Property("CascadeNearPlaneOffset", CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
				UI::Property("CascadeFarPlaneOffset", CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			if (UI::BeginTreeNode("Shadow Map", false))
			{
				static int cascadeIndex = 0;
				auto fb = ShadowMapRenderPass[cascadeIndex]->GetSpecification().TargetFramebuffer;
				auto image = fb->GetDepthImage();

				float size = ImGui::GetContentRegionAvail().x; // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
				UI::BeginPropertyGrid();
				UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
				UI::EndPropertyGrid();
				if (m_ResourcesCreated)
					UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Compute Bloom"))
		{
			float size = ImGui::GetContentRegionAvail().x;
			if (m_ResourcesCreated)
			{
				static int tex = 0;
				UI::PropertySlider("Texture", tex, 0, 2);
				static int mip = 0;
				auto [mipWidth, mipHeight] = m_BloomComputeTextures[tex]->GetMipSize(mip);
				std::string label = std::format("Mip ({0}x{1})", mipWidth, mipHeight);
				UI::PropertySlider(label.c_str(), mip, 0, m_BloomComputeTextures[tex]->GetMipLevelCount() - 1);
				//UI::ImageMip(m_BloomComputeTextures[tex]->GetImage(), mip, { size, size * (1.0f / m_BloomComputeTextures[tex]->GetImage()->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
			}
			UI::EndTreeNode();
		}

		ImGui::End();
	}

}