#pragma once

#include "Base.hpp"

#define VA_VERSION "v0.1.0a"

// ==== Build Configuration ====
#if defined(VA_DEBUG)
    #define VA_BUILD_CONFIG_NAME "Debug"
#elif defined(VA_RELEASE)
    #define VA_BUILD_CONFIG_NAME "Release"
#else
    #define VA_BUILD_CONFIG_NAME "Unknown"
#endif

// ==== Build Platform ====
#if defined(VA_PLATFORM_WINDOWS)
    #define VA_BUILD_PLATFORM_NAME "Windows x64"
#elif defined(VA_PLATFORM_LINUX)
    #define VA_BUILD_PLATFORM_NAME "Linux"
#else
    #define VA_BUILD_PLATFORM_NAME "Unknown"
#endif

#define VA_VERSION_LONG "Vanta " VA_VERSION " (" VA_BUILD_PLATFORM_NAME " " VA_BUILD_CONFIG_NAME ")"

// Stable build version (YEAR.SEASON.MAJOR.MINOR) Season is 1(Winter), 2(Spring), 3(Summer), 4(Fall)