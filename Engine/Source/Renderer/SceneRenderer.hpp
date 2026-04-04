#pragma once

#include "Scene/Scene.hpp"
#include "Scene/Components.hpp"
#include "Renderer/Mesh.hpp"
#include "RenderPass.hpp"

#include "Renderer/UniformBufferSet.hpp"
#include "Renderer/RenderCommandBuffer.hpp"
#include "Renderer/PipelineCompute.hpp"

#include <map>

#include "StorageBufferSet.hpp"

namespace Vanta {

	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowSelectedInWireframe = false;
	};

	struct SceneRendererCamera
	{
		Vanta::Camera Camera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	struct BloomSettings
	{
		bool Enabled = true;
		float Threshold = 1.0f;
		float Knee = 0.1f;
		float UpsampleScale = 1.0f;
		float Intensity = 1.0f;
		float DirtIntensity = 1.0f;
	};

	struct SceneRendererSpecification
	{
		bool SwapChainTarget = false;
	};

	class SceneRenderer : public RefCounted
	{
	public:
		SceneRenderer(Ref<Scene> scene, SceneRendererSpecification specification = SceneRendererSpecification());
		~SceneRenderer();

		void Init();

		void SetScene(Ref<Scene> scene);

		void SetViewportSize(uint32_t width, uint32_t height);

		void BeginScene(const SceneRendererCamera& camera);
		void EndScene();

		void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = nullptr);
		void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		Ref<RenderPass> GetFinalRenderPass();
		Ref<RenderPass> GetExternalCompositeRenderPass() { return m_ExternalCompositeRenderPass; }
		Ref<Image2D> GetFinalPassImage();

		SceneRendererOptions& GetOptions();

		void SetShadowSettings(float nearPlane, float farPlane, float lambda)
		{
			CascadeNearPlaneOffset = nearPlane;
			CascadeFarPlaneOffset = farPlane;
			CascadeSplitLambda = lambda;
		}

		BloomSettings& GetBloomSettings() { return m_BloomSettings; }

		void OnImGuiRender();

		static void WaitForThreads();
	private:
		void FlushDrawList();

		void ClearPass();
		void ShadowMapPass();
		void GeometryPass();
		void JumpFloodPass();

		// Post-processing
		void BloomCompute();
		void CompositePass();

		struct CascadeData
		{
			glm::mat4 ViewProj;
			glm::mat4 View;
			float SplitDepth;
		};
		void CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection) const;
	private:
		Ref<Scene> m_Scene;
		SceneRendererSpecification m_Specification;
		Ref<RenderCommandBuffer> m_CommandBuffer;

		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			// Resources
			Ref<Environment> SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
			LightEnvironment SceneLightEnvironment;
			DirLight ActiveLight;
		} m_SceneData;

		Ref<Shader> m_CompositeShader;
		Ref<Shader> m_BloomBlurShader;
		Ref<Shader> m_BloomBlendShader;

		struct UBCamera
		{
			glm::mat4 ViewProjection;
			glm::mat4 InverseViewProjection;
			glm::mat4 Projection;
			glm::mat4 View;
		} CameraData;

		struct UBShadow
		{
			glm::mat4 ViewProjection[4];
		} ShadowData;

		struct DirLight
		{
			glm::vec3 Direction;
			float Padding = 0.0f;
			glm::vec3 Radiance;
			float Multiplier;
		};

		struct UBScene
		{
			DirLight lights;
			glm::vec3 u_CameraPosition;
			float EnvironmentMapIntensity = 1.0f;
		} SceneDataUB;

		struct UBRendererData
		{
			glm::vec4 CascadeSplits;
			bool ShowCascades = false;
			char Padding0[3] = { 0,0,0 }; // Bools are 4-bytes in GLSL
			bool SoftShadows = true;
			char Padding1[3] = { 0,0,0 };
			float LightSize = 0.5f;
			float MaxShadowDistance = 200.0f;
			float ShadowFade = 1.0f;
			bool CascadeFading = true;
			char Padding2[3] = { 0,0,0 };
			float CascadeTransitionFade = 1.0f;
			char Padding3[3] = { 0,0,0 };
		} RendererDataUB;

		Ref<UniformBufferSet> m_UniformBufferSet;
		Ref<StorageBufferSet> m_StorageBufferSet;

		Ref<Shader> ShadowMapShader, ShadowMapAnimShader;
		Ref<RenderPass> ShadowMapRenderPass[4];
		float LightDistance = 0.1f;
		float CascadeSplitLambda = 0.92f;
		glm::vec4 CascadeSplits;
		float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;

		bool EnableBloom = false;
		float BloomThreshold = 1.5f;

		glm::vec2 FocusPoint = { 0.5f, 0.5f };

		RendererID ShadowMapSampler;
		Ref<Material> CompositeMaterial;

		Ref<Pipeline> m_GeometryPipeline;
		Ref<Pipeline> m_SelectedGeometryPipeline;
		Ref<Pipeline> m_GeometryWireframePipeline;
		Ref<Pipeline> m_CompositePipeline;
		Ref<Pipeline> m_ShadowPassPipelines[4];
		Ref<Material> m_ShadowPassMaterial;
		Ref<Pipeline> m_SkyboxPipeline;
		Ref<Material> m_SkyboxMaterial;

		// Jump Flood Pass
		Ref<Pipeline> m_JumpFloodInitPipeline;
		Ref<Pipeline> m_JumpFloodPassPipeline[2];
		Ref<Pipeline> m_JumpFloodCompositePipeline;
		Ref<Material> m_JumpFloodInitMaterial, m_JumpFloodPassMaterial[2];
		Ref<Material> m_JumpFloodCompositeMaterial;

		// Bloom compute
		uint32_t m_BloomComputeWorkgroupSize = 4;
		Ref<PipelineCompute> m_BloomComputePipeline;
		Ref<Texture2D> m_BloomComputeTextures[3];
		Ref<Material> m_BloomComputeMaterial;

		Ref<Material> m_SelectedGeometryMaterial;

		std::vector<Ref<Framebuffer>> m_TempFramebuffers;

		Ref<RenderPass> m_ExternalCompositeRenderPass;

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<Material> Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> m_DrawList;
		std::vector<DrawCommand> m_SelectedMeshDrawList;
		std::vector<DrawCommand> m_ShadowPassDrawList;

		// Grid
		Ref<Pipeline> m_GridPipeline;
		Ref<Shader> m_GridShader;
		Ref<Material> m_GridMaterial;
		Ref<Material> m_OutlineMaterial, OutlineAnimMaterial;
		Ref<Material> m_WireframeMaterial;

		SceneRendererOptions m_Options;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_NeedsResize = false;
		bool m_Active = false;
		bool m_ResourcesCreated = false;

		BloomSettings m_BloomSettings;
		Ref<Texture2D> m_BloomDirtTexture;

		struct GPUTimeQueries
		{
			uint32_t ShadowMapPassQuery = 0;
			uint32_t GeometryPassQuery = 0;
			uint32_t BloomComputePassQuery = 0;
			uint32_t JumpFloodPassQuery = 0;
			uint32_t CompositePassQuery = 0;
		} m_GPUTimeQueries;
	};

}