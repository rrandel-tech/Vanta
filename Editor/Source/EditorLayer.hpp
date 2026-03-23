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

        std::unique_ptr<VertexBuffer> m_VB;
        std::unique_ptr<IndexBuffer> m_IB;
        std::unique_ptr<Shader> m_Shader;
        glm::vec4 m_TriangleColor;
    };

}