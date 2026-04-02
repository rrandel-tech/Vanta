#pragma once

#include "Project.hpp"

#include <string>

namespace Vanta {

    class ProjectSerializer
    {
    public:
        ProjectSerializer(Ref<Project> project);

        void Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);
    private:
        Ref<Project> m_Project;
    };

}