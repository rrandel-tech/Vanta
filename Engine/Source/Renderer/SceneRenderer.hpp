#pragma once

#include "Scene/Scene.hpp"
#include "Scene/Components.hpp"
#include "Renderer/Mesh.hpp"
#include "RenderPass.hpp"

namespace Vanta {

	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowBoundingBoxes = false;
	};

	struct SceneRendererCamera
	{
		Vanta::Camera Camera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	class SceneRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(const Scene* scene, const SceneRendererCamera& camera);
		static void EndScene();

		static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = nullptr);
		static void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath);

		static Ref<RenderPass> GetFinalRenderPass();
		static Ref<Image2D> GetFinalPassImage();

		static SceneRendererOptions& GetOptions();

		static void OnImGuiRender();
	private:
		static void FlushDrawList();
		static void ShadowMapPass();
		static void GeometryPass();
		static void CompositePass();
		static void BloomBlurPass();
	};

}
