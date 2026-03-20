#pragma once

#include "Core/Application.hpp"
#include "Core/Assert.hpp"

int main(int argc, char** argv)
{
    Vanta::InitializeCore();
    Vanta::Application* app = Vanta::CreateApplication(argc, argv);
    VA_CORE_ASSERT(app, "Client Application is null!");
    app->Run();
    delete app;
    Vanta::ShutdownCore();
}