#pragma once

#include <memory>

namespace Vanta {
    void InitializeCore();
    void ShutdownCore();
};

#if defined(_WIN64) || defined(_WIN32)
#define VA_PLATFORM_WINDOWS
#elif defined(__linux__)
#define VA_PLATFORM_LINUX
#else
#error "Unsupported platform! Vanta supports Windows and Linux."
#endif

#ifdef NDEBUG
#define VA_RELEASE
#else
#define VA_DEBUG
#endif

#define BIT(x) (1u << x)

//------------------------------------------------------------------------------
// Compiler Detection
//------------------------------------------------------------------------------

#if defined(__clang__)
#define VA_COMPILER_CLANG
#elif defined(__GNUC__)
#define VA_COMPILER_GCC
#elif defined(_MSC_VER)
#define VA_COMPILER_MSVC
#else
#error "Unknown compiler! Vanta only supports MSVC, GCC, and Clang."
#endif

// Pointer wrappers
namespace Vanta {

    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    using byte = uint8_t;

}