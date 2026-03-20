#include "EntryPoint.hpp"

class VantaEditorApplication : public Vanta::Application
{
public:
    VantaEditorApplication(const Vanta::ApplicationSpecification& specification)
        : Application(specification)
    {
    }
};

Vanta::Application* Vanta::CreateApplication(int argc, char** argv)
{
    ApplicationSpecification specification;
    specification.name = "Vanta-Editor";
    specification.windowWidth = 1280;
    specification.windowHeight = 720;

    return new VantaEditorApplication(specification);
}