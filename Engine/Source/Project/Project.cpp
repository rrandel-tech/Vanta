#include "vapch.hpp"
#include "Project.hpp"

#include "Asset/AssetManager.hpp"

namespace Vanta {

    Project::Project()
    {
    }

    Project::~Project()
    {
    }

    void Project::SetActive(Ref<Project> project)
    {
        s_ActiveProject = project;
        AssetManager::Init();
    }

    void Project::OnDeserialized()
    {
    }

}