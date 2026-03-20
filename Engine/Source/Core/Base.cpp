#include "vapch.hpp"
#include "Base.hpp"

#include "Log.hpp"

namespace Vanta {

    void InitializeCore()
    {
        Log::Init();

        VA_CORE_TRACE("Vanta Engine {}", VA_VERSION);
        VA_CORE_TRACE("Initializing...");
    }

    void ShutdownCore()
    {
        VA_CORE_TRACE("Shutting down...");
        Log::Shutdown();
    }

}