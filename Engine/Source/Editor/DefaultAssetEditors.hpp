#pragma once

#include "AssetEditorPanel.hpp"
#include "Renderer/Mesh.hpp"

namespace Vanta {

	class TextureViewer : public AssetEditor
	{
	public:
		TextureViewer();

		virtual void SetAsset(const Ref<Asset>& asset) override { m_Asset = (Ref<Texture>)asset; }

	private:
		virtual void Render() override;

	private:
		Ref<Texture> m_Asset;
	};

}
