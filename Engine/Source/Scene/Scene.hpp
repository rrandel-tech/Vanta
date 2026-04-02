#pragma once

#include "Core/UUID.hpp"
#include "Core/Timestep.hpp"

#include "Renderer/Camera.hpp"
#include "Renderer/Texture.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/SceneEnvironment.hpp"

#include "entt/entt.hpp"

#include "SceneCamera.hpp"
#include "Editor/EditorCamera.hpp"

namespace Vanta {

	class SceneRenderer;

	struct Light
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		float Multiplier = 1.0f;
	};

	struct DirectionalLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float Multiplier = 0.0f;

		// C++ only
		bool CastShadows = true;
	};

	struct LightEnvironment
	{
		DirectionalLight DirectionalLights[4];
	};

	class Entity;
	using EntityMap = std::unordered_map<UUID, Entity>;

	struct TransformComponent;

	class Scene : public Asset
	{
	public:
		Scene(const std::string& debugName = "Scene", bool isEditorScene = false);
		~Scene();

		void Init();

		void OnUpdate(Timestep ts);
		void OnRenderRuntime(Ref<SceneRenderer> renderer, Timestep ts);
		void OnRenderEditor(Ref<SceneRenderer> renderer, Timestep ts, const EditorCamera& editorCamera);
		void OnEvent(Event& e);

		// Runtime
		void OnRuntimeStart();
		void OnRuntimeStop();

		void SetViewportSize(uint32_t width, uint32_t height);

		const Ref<Environment>& GetEnvironment() const { return m_Environment; }
		void SetSkybox(const Ref<TextureCube>& skybox);

		Light& GetLight() { return m_Light; }
		const Light& GetLight() const { return m_Light; }

		Entity GetMainCameraEntity();

		float& GetSkyboxLod() { return m_SkyboxLod; }
		float GetSkyboxLod() const { return m_SkyboxLod; }

		Entity CreateEntity(const std::string& name = "");
		Entity CreateEntityWithID(UUID uuid, const std::string& name = "", bool runtimeMap = false);
		void DestroyEntity(Entity entity);

		void DuplicateEntity(Entity entity);

		template<typename T>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<T>();
		}

		Entity FindEntityByTag(const std::string& tag);
		Entity FindEntityByUUID(UUID id);

		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);
		glm::mat4 GetTransformRelativeToParent(Entity entity);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		TransformComponent GetWorldSpaceTransform(Entity entity);

		void ParentEntity(Entity entity, Entity parent);
		void UnparentEntity(Entity entity);

		const EntityMap& GetEntityMap() const { return m_EntityIDMap; }
		void CopyTo(Ref<Scene>& target);

		UUID GetUUID() const { return m_SceneID; }

		static Ref<Scene> GetScene(UUID uuid);

		// Editor-specific
		void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }

		static AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }
	private:
		UUID m_SceneID;
		entt::entity m_SceneEntity;
		entt::registry m_Registry;

		std::string m_Name = "UntitledScene";
		std::string m_DebugName;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		EntityMap m_EntityIDMap;

		Light m_Light;
		float m_LightMultiplier = 0.3f;

		LightEnvironment m_LightEnvironment;

		Ref<Environment> m_Environment;
		float m_EnvironmentIntensity = 1.0f;
		Ref<TextureCube> m_SkyboxTexture;
		Ref<Material> m_SkyboxMaterial;

		entt::entity m_SelectedEntity;

		float m_SkyboxLod = 1.0f;
		bool m_IsPlaying = false;

		friend class Entity;
		friend class SceneRenderer;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}