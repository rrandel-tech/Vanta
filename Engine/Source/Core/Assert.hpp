#pragma once

#include "Base.hpp"
#include "Log.hpp"

#include "Version.hpp"

#ifdef VA_PLATFORM_WINDOWS
    #define VA_DEBUG_BREAK __debugbreak()
#elif defined(VA_COMPILER_CLANG)
    #define VA_DEBUG_BREAK __builtin_debugtrap()
#else
    #define VA_DEBUG_BREAK
#endif

#ifdef VA_DEBUG
    #define VA_ENABLE_ASSERTS
    #define VA_EXPAND_VARGS(x) x
#endif

#ifdef VA_ENABLE_ASSERTS
    #define VA_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { VA_ERROR("Assertion Failed"); VA_DEBUG_BREAK; } }
    #define VA_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { VA_ERROR("Assertion Failed: {0}", __VA_ARGS__); VA_DEBUG_BREAK; } }

    #define VA_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro
    #define VA_GET_ASSERT_MACRO(...) VA_EXPAND_VARGS(VA_ASSERT_RESOLVE(__VA_ARGS__, VA_ASSERT_MESSAGE, VA_ASSERT_NO_MESSAGE))

    #define VA_ASSERT(...) VA_EXPAND_VARGS( VA_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
    #define VA_CORE_ASSERT(...) VA_EXPAND_VARGS( VA_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
    #define VA_ASSERT(...)
    #define VA_CORE_ASSERT(...)
#endif

#ifdef VA_ENABLE_VERIFY
    #define VA_VERIFY_NO_MESSAGE(condition) { if(!(condition)) { VA_ERROR("Verify Failed"); __debugbreak(); } }
    #define VA_VERIFY_MESSAGE(condition, ...) { if(!(condition)) { VA_ERROR("Verify Failed: {0}", __VA_ARGS__); __debugbreak(); } }

    #define VA_VERIFY_RESOLVE(arg1, arg2, macro, ...) macro
    #define VA_GET_VERIFY_MACRO(...) VA_EXPAND_VARGS(VA_VERIFY_RESOLVE(__VA_ARGS__, VA_VERIFY_MESSAGE, VA_VERIFY_NO_MESSAGE))

    #define VA_VERIFY(...) VA_EXPAND_VARGS( VA_GET_VERIFY_MACRO(__VA_ARGS__)(__VA_ARGS__) )
    #define VA_CORE_VERIFY(...) VA_EXPAND_VARGS( VA_GET_VERIFY_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
    #define VA_VERIFY(...)
    #define VA_CORE_VERIFY(...)
#endif