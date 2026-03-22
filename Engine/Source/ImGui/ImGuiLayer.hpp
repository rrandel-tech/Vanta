#pragma once

#include "Core/Layer.hpp"

namespace Vanta {

    class ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ImGuiLayer(const std::string& name);
        virtual ~ImGuiLayer();

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(Timestep ts) override;
    private:
        float m_Time = 0.0f;
    };

}