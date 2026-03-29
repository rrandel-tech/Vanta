#pragma once

#include "Renderer/Texture.hpp"
#include "Asset/AssetManager.hpp"

namespace Vanta {

	class ObjectsPanel
	{
	public:
		ObjectsPanel();

		void OnImGuiRender();

	private:
		void DrawObject(const char* label, AssetHandle handle);

	private:
		Ref<Texture2D> m_CubeImage;
	};

}
