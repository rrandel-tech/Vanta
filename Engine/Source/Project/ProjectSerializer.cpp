#include "vapch.hpp"
#include "ProjectSerializer.hpp"

#include "yaml-cpp/yaml.h"

#include <filesystem>

namespace Vanta {

    ProjectSerializer::ProjectSerializer(Ref<Project> project)
        : m_Project(project)
    {
    }

    void ProjectSerializer::Serialize(const std::string& filepath)
    {
        VA_CORE_ASSERT(false);
    }

    bool ProjectSerializer::Deserialize(const std::string& filepath)
    {
        std::ifstream stream(filepath);
        VA_CORE_ASSERT(stream);
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        if (!data["Project"])
            return false;

        YAML::Node rootNode = data["Project"];
        if (!rootNode["Name"])
            return false;

        auto& config = m_Project->m_Config;
        config.Name = rootNode["Name"].as<std::string>();

        config.AssetDirectory = rootNode["AssetDirectory"].as<std::string>();
        config.AssetRegistryPath = rootNode["AssetRegistry"].as<std::string>();

        config.MeshPath = rootNode["MeshPath"].as<std::string>();
        config.MeshSourcePath = rootNode["MeshSourcePath"].as<std::string>();

        config.StartScene = rootNode["StartScene"].as<std::string>();

        std::filesystem::path projectPath = filepath;
        config.ProjectDirectory = projectPath.parent_path().string();

        m_Project->OnDeserialized();

        return true;
    }

}