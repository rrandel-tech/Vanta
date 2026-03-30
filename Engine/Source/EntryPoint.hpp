#pragma once

#include "Core/Application.hpp"
#include "Core/Assert.hpp"

bool g_ApplicationRunning = true;

int main(int argc, char** argv)
{
    while (g_ApplicationRunning)
    {
        Vanta::InitializeCore();
        Vanta::Application* app = Vanta::CreateApplication(argc, argv);
        VA_CORE_ASSERT(app, "Client Application is null!");
        app->Run();
        delete app;
        Vanta::ShutdownCore();
    }
}