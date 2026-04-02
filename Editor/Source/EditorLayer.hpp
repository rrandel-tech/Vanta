#pragma once

#include "Vanta.hpp"

#include "ImGui/ImGuiLayer.hpp"
#include "Editor/EditorCamera.hpp"
#include "imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "Editor/SceneHierarchyPanel.hpp"
#include "Editor/ContentBrowserPanel.hpp"
#include "Editor/ObjectsPanel.hpp"

namespace Vanta {

	class EditorLayer : public Layer
	{
	public:
		enum class PropertyFlag
		{
			None = 0, ColorProperty = 1, DragProperty = 2, SliderProperty = 4
		};
	public:
		EditorLayer();
		virtual ~EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;

		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void ShowBoundingBoxes(bool show, bool onTop = false);
		void SelectEntity(Entity entity);

		void NewScene();
		void OpenProject();
		void OpenProject(const std::string& filepath);
		void OpenScene(const std::string& filepath);
		void OpenScene(const AssetMetadata& assetMetadata);
		void SaveScene();
		void SaveSceneAs();

		void UI_WelcomePopup();
		void UI_AboutPopup();
	private:
		std::pair<float, float> GetMouseViewportSpace(bool primaryViewport);
		std::pair<glm::vec3, glm::vec3> CastRay(const EditorCamera& camera, float mx, float my);

		struct SelectedSubmesh
		{
			Vanta::Entity Entity;
			Submesh* Mesh = nullptr;
			float Distance = 0.0f;
		};

		void OnSelected(const SelectedSubmesh& selectionContext);
		void OnEntityDeleted(Entity e);
		Ray CastMouseRay();

		void OnScenePlay();
		void OnSceneStop();

		void UpdateWindowTitle(const std::string& sceneName);

		float GetSnapValue();

		void DeleteEntity(Entity entity);

		void UpdateSceneRendererSettings();
	private:
		Scope<SceneHierarchyPanel> m_SceneHierarchyPanel;
		Scope<ContentBrowserPanel> m_ContentBrowserPanel;
		Scope<ObjectsPanel> m_ObjectsPanel;

		Ref<Scene> m_RuntimeScene, m_EditorScene, m_CurrentScene;
		Ref<SceneRenderer> m_ViewportRenderer;
		Ref<SceneRenderer> m_SecondViewportRenderer;
		Ref<SceneRenderer> m_FocusedRenderer;
		std::string m_SceneFilePath;

		EditorCamera m_EditorCamera;
		EditorCamera m_SecondEditorCamera;

		Ref<Shader> m_BrushShader;
		Ref<Material> m_SphereBaseMaterial;

		float m_LineWidth = 2.0f;

		struct AlbedoInput
		{
			glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
			Ref<Texture2D> TextureMap;
			bool SRGB = true;
			bool UseTexture = false;
		};
		//AlbedoInput m_AlbedoInput;

		struct NormalInput
		{
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		//NormalInput m_NormalInput;

		struct MetalnessInput
		{
			float Value = 1.0f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		//MetalnessInput m_MetalnessInput;

		struct RoughnessInput
		{
			float Value = 0.2f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		//RoughnessInput m_RoughnessInput;

		float m_EnvMapRotation = 0.0f;

		enum class SceneType : uint32_t
		{
			Spheres = 0, Model = 1
		};
		SceneType m_SceneType;

		// Editor resources
		Ref<Texture2D> m_CheckerboardTex;
		Ref<Texture2D> m_PlayButtonTex, m_StopButtonTex, m_PauseButtonTex;

		glm::vec2 m_ViewportBounds[2];
		glm::vec2 m_SecondViewportBounds[2];
		int m_GizmoType = -1; // -1 = no gizmo
		float m_SnapValue = 0.5f;
		float m_RotationSnapValue = 45.0f;
		bool m_DrawOnTopBoundingBoxes = false;

		bool m_ShowBoundingBoxes = false;
		bool m_ShowBoundingBoxSelectedMeshOnly = true;
		bool m_ShowBoundingBoxSubmeshes = false;
		bool m_ShowSelectedWireframe = true;

		bool m_ViewportPanelMouseOver = false;
		bool m_ViewportPanelFocused = false;

		bool m_ViewportPanel2MouseOver = false;
		bool m_ViewportPanel2Focused = false;

		bool m_ShowSecondViewport = false;

		bool m_ShowWelcomePopup = true;
		bool m_ShowAboutPopup = false;

		enum class SceneState
		{
			Edit = 0, Play = 1, Pause = 2
		};
		SceneState m_SceneState = SceneState::Edit;

		enum class SelectionMode
		{
			None = 0, Entity = 1, SubMesh = 2
		};

		SelectionMode m_SelectionMode = SelectionMode::Entity;
		std::vector<SelectedSubmesh> m_SelectionContext;
		glm::mat4* m_RelativeTransform = nullptr;
		glm::mat4* m_CurrentlySelectedTransform = nullptr;
	};

}