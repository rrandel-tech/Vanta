#include "EditorLayer.hpp"

#include "EntryPoint.hpp"

class VantaEditorApplication : public Vanta::Application
{
public:
    VantaEditorApplication(const Vanta::ApplicationSpecification& specification)
        : Application(specification)
    {
    }

    virtual void OnInit() override
    {
        PushLayer(new Vanta::EditorLayer());
    }
};

Vanta::Application* Vanta::CreateApplication(int argc, char** argv)
{
    ApplicationSpecification specification;
    specification.Name = "Vanta-Editor";
    specification.WindowWidth = 1280;
    specification.WindowHeight = 720;
    specification.Mode = WindowMode::Windowed;

    return new VantaEditorApplication(specification);
}