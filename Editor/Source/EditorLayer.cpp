#include "EditorLayer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
        : m_ClearColor{ 0.1f, 0.1f, 0.1f, 1.0f }, m_TriangleColor{ 0.75f, 0.45f, 0.50f, 1.0f }
    {
    }

    EditorLayer::~EditorLayer()
    {
    }

    void EditorLayer::OnAttach()
    {
        static float vertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f
        };

        static uint32_t indices[] = {
            0, 1, 2
        };

        m_VB = std::unique_ptr<VertexBuffer>(VertexBuffer::Create());
        m_VB->SetData(vertices, sizeof(vertices));

        m_IB = std::unique_ptr<IndexBuffer>(IndexBuffer::Create());
        m_IB->SetData(indices, sizeof(indices));

        m_Shader.reset(Shader::Create("C:\\Development\\Vanta\\Editor\\assets\\shaders\\shader.glsl"));
    }

    void EditorLayer::OnDetach()
    {
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        Renderer::Clear(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);

        UniformBufferDeclaration<sizeof(glm::vec4), 1> buffer;
        buffer.Push("u_Color", m_TriangleColor);
        m_Shader->UploadUniformBuffer(buffer);

        m_Shader->Bind();
        m_VB->Bind();
        m_IB->Bind();
        Renderer::DrawIndexed(6);
    }

    void EditorLayer::OnImGuiRender()
    {
        ImGui::Begin("EditorLayer");
        ImGui::ColorEdit4("Clear Color", m_ClearColor);
        ImGui::ColorEdit4("Triangle Color", glm::value_ptr(m_TriangleColor));
        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& event)
    {
        if (Input::IsKeyPressed(SDL_SCANCODE_W))
            VA_INFO("W key pressed from app!");
    }

}