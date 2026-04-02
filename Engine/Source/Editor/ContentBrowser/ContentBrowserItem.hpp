#pragma once

#include "Core/Input.hpp"
#include "Renderer/Texture.hpp"
#include "Asset/AssetMetadata.hpp"

namespace Vanta {

#define MAX_INPUT_BUFFER_LENGTH 128

	enum class ContentBrowserAction
	{
		None				= 0,
		Refresh				= BIT(0),
		ClearSelections		= BIT(1),
		Selected			= BIT(2),
		DeSelected			= BIT(3),
		Hovered				= BIT(4),
		Renamed				= BIT(5),
		NavigateToThis		= BIT(6),
		OpenDeleteDialogue	= BIT(7),
		SelectToHere		= BIT(8),
		Moved				= BIT(9),
		ShowInExplorer		= BIT(10),
		OpenExternal        = BIT(11),
		Reload              = BIT(12)
	};

	struct CBItemActionResult
	{
		uint16_t Field = 0;

		void Set(ContentBrowserAction flag, bool value)
		{
			if (value)
				Field |= (uint16_t)flag;
			else
				Field &= ~(uint16_t)flag;
		}

		bool IsSet(ContentBrowserAction flag) const { return (uint16_t)flag & Field; }
	};

	class ContentBrowserItem : public RefCounted
	{
	public:
		enum class ItemType : uint16_t
		{
			Directory, Asset
		};
	public:
		ContentBrowserItem(ItemType type, AssetHandle id, const std::string& name, const Ref<Texture2D>& icon);
		virtual ~ContentBrowserItem() {}

		void OnRenderBegin();
		CBItemActionResult OnRender();
		void OnRenderEnd();

		virtual void Delete() {}
		virtual bool Move(const std::filesystem::path& destination) { return false; }

		bool IsSelected() const { return m_IsSelected; }

		AssetHandle GetID() const { return m_ID; }
		ItemType GetType() const { return m_Type; }
		const std::string& GetName() const { return m_Name; }

		const Ref<Texture2D>& GetIcon() const { return m_Icon; }

		virtual void Activate(CBItemActionResult& actionResult) {}
		void StartRenaming();

		void SetSelected(bool value);

		void Rename(const std::string& newName, bool fromCallback = false);

	private:
		virtual void OnRenamed(const std::string& newName, bool fromCallback = false) {}
		virtual void RenderCustomContextItems() {}
		virtual void UpdateDrop(CBItemActionResult& actionResult) {}

		void OnContextMenuOpen(CBItemActionResult& actionResult);

	private:
		ItemType m_Type;
		AssetHandle m_ID;
		std::string m_Name;
		Ref<Texture2D> m_Icon;

		bool m_IsSelected = false;
		bool m_IsRenaming = false;
		bool m_IsDragging = false;

	private:
		friend class ContentBrowserPanel;
	};

	struct DirectoryInfo : public RefCounted
	{
		AssetHandle Handle;
		Ref<DirectoryInfo> Parent;

		std::string Name;
		std::filesystem::path FilePath;

		std::vector<AssetHandle> Assets;
		std::unordered_map<AssetHandle, Ref<DirectoryInfo>> SubDirectories;
	};

	class ContentBrowserDirectory : public ContentBrowserItem
	{
	public:
		ContentBrowserDirectory(const Ref<DirectoryInfo>& directoryInfo, const Ref<Texture2D>& icon);
		virtual ~ContentBrowserDirectory();

		Ref<DirectoryInfo>& GetDirectoryInfo() { return m_DirectoryInfo; }

		virtual void Delete() override;
		virtual bool Move(const std::filesystem::path& destination) override;

	private:
		virtual void Activate(CBItemActionResult& actionResult) override;
		virtual void OnRenamed(const std::string& newName, bool fromCallback) override;
		virtual void UpdateDrop(CBItemActionResult& actionResult) override;

		void UpdateDirectoryPath(Ref<DirectoryInfo> directoryInfo, const std::filesystem::path& newParentPath);

	private:
		Ref<DirectoryInfo> m_DirectoryInfo;
	};

	class ContentBrowserAsset : public ContentBrowserItem
	{
	public:
		ContentBrowserAsset(const AssetMetadata& assetInfo, const Ref<Texture2D>& icon);
		virtual ~ContentBrowserAsset();

		const AssetMetadata& GetAssetInfo() const { return m_AssetInfo; }

		virtual void Delete() override;
		virtual bool Move(const std::filesystem::path& destination) override;

	private:
		virtual void Activate(CBItemActionResult& actionResult) override;
		virtual void OnRenamed(const std::string& newName, bool fromCallback) override;

	private:
		AssetMetadata m_AssetInfo;
	};

	namespace Utils
	{
		static std::string ContentBrowserItemTypeToString(ContentBrowserItem::ItemType type)
		{
			switch (type)
			{
			case ContentBrowserItem::ItemType::Asset: return "Asset";
			case ContentBrowserItem::ItemType::Directory: return "Directory";
			}

			return "Unknown";
		}
	}

}