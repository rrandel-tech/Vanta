#include "EditorLayer.hpp"

static void ImGuiShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

namespace Vanta {

    EditorLayer::EditorLayer()
        : m_ClearColor{0.2f, 0.3f, 0.8f, 1.0f}
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
        Vanta::Renderer::Clear(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
    }

    void EditorLayer::OnImGuiRender()
    {
        static bool show_demo_window = true;
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::Begin("GameLayer");
        ImGui::ColorEdit4("Clear Color", m_ClearColor);
        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& event)
    {
        if (Input::IsKeyPressed(SDL_SCANCODE_W))
            VA_INFO("W key pressed from app!");
    }

}