#pragma once

#include "Scene/Scene.hpp"
#include "Scene/Entity.hpp"
#include "Renderer/Mesh.hpp"

namespace Vanta {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);
		void SetSelected(Entity entity);
		void SetSelectionChangedCallback(const std::function<void(Entity)>& func) { m_SelectionChangedCallback = func; }
		void SetEntityDeletedCallback(const std::function<void(Entity)>& func) { m_EntityDeletedCallback = func; }

		void OnImGuiRender();
	private:
		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		std::function<void(Entity)> m_SelectionChangedCallback, m_EntityDeletedCallback;
	};

}