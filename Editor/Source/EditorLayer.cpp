#include "EditorLayer.hpp"

namespace Vanta {

    EditorLayer::EditorLayer()
    {
    }

    EditorLayer::~EditorLayer()
    {
    }

    void EditorLayer::OnAttach()
    {
        VA_INFO("Hello from editor!");
    }

    void EditorLayer::OnDetach()
    {
    }

    void EditorLayer::OnUpdate()
    {
    }

    void EditorLayer::OnEvent(Event& e)
    {
    }

}