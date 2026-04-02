#pragma once

#include "Core/Ref.hpp"

#include <filesystem>

#include "Core/Assert.hpp"

namespace Vanta {

	struct ProjectConfig
	{
		std::string Name;

		std::string AssetDirectory;
		std::string AssetRegistryPath;

		std::string MeshPath;
		std::string MeshSourcePath;

		std::string StartScene;

		// Not serialized
		std::string ProjectDirectory;
	};

	class Project : public RefCounted
	{
	public:
		Project();
		~Project();

		const ProjectConfig& GetConfig() const { return m_Config; }

		static Ref<Project> GetActive() { return s_ActiveProject; }
		static void SetActive(Ref<Project> project);

		static const std::string& GetProjectName()
		{
			VA_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetConfig().Name;
		}

		static std::filesystem::path GetProjectDirectory()
		{
			VA_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->GetConfig().ProjectDirectory;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			VA_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().AssetDirectory;
		}

		static std::filesystem::path GetAssetRegistryPath()
		{
			VA_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().AssetRegistryPath;
		}

		static std::filesystem::path GetMeshPath()
		{
			VA_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / s_ActiveProject->GetConfig().MeshPath;
		}

		static std::filesystem::path GetCacheDirectory()
		{
			VA_CORE_ASSERT(s_ActiveProject);
			return std::filesystem::path(s_ActiveProject->GetConfig().ProjectDirectory) / "Cache";
		}
	private:
		void OnDeserialized();
	private:
		ProjectConfig m_Config;

		friend class ProjectSerializer;

		inline static Ref<Project> s_ActiveProject;
	};

}
