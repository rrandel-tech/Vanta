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

    void EditorLayer::OnUpdate(Timestep ts)
    {
    }

    void EditorLayer::OnEvent(Event& e)
    {
        if (Input::IsKeyPressed(SDL_SCANCODE_W))
            VA_INFO("W key pressed from app!");
    }

}