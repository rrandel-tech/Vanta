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
        virtual void OnUpdate() override;
        virtual void OnEvent(Event& e) override;
    };

}