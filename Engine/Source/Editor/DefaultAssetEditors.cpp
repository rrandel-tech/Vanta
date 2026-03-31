#include "vapch.hpp"
#include "DefaultAssetEditors.hpp"
#include "Asset/AssetImporter.hpp"
#include "Asset/AssetManager.hpp"

namespace Vanta {

	TextureViewer::TextureViewer()
		: AssetEditor("Edit Texture")
	{
		SetMinSize(200, 600);
		SetMaxSize(500, 1000);
	}

	void TextureViewer::OnClose()
	{
		m_Asset = nullptr;
	}

	void TextureViewer::Render()
	{
		if (!m_Asset)
			SetOpen(false);

		float textureWidth = m_Asset->GetWidth();
		float textureHeight = m_Asset->GetHeight();
		//float bitsPerPixel = Texture::GetBPP(m_Asset->GetFormat());
		float imageSize = ImGui::GetWindowWidth() - 40;
		imageSize = glm::min(imageSize, 500.0f);

		ImGui::SetCursorPosX(20);
		//ImGui::Image((ImTextureID)m_Asset->GetRendererID(), { imageSize, imageSize });

		UI::BeginPropertyGrid();
		UI::Property("Width", textureWidth, 0.1f, 0.0f, 0.0f, true);
		UI::Property("Height", textureHeight, 0.1f, 0.0f, 0.0f, true);
		// UI::Property("Bits", bitsPerPixel, 0.1f, 0.0f, 0.0f, true); // TODO: Format
		UI::EndPropertyGrid();
	}

}