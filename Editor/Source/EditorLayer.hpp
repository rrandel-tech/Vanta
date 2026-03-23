#pragma once

#include "Vanta.hpp"

namespace Vanta {

    class EditorLayer : public Layer
    {
    public:
        EditorLayer();
        virtual ~EditorLayer();

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(Timestep ts) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Event& e) override;
    private:
        float m_ClearColor[4];
    };

}