#pragma once

#define VA_ENABLE_PROFILING !VA_DIST

#if VA_ENABLE_PROFILING 
#include <Tracy.hpp>
#endif

#if VA_ENABLE_PROFILING
#define VA_PROFILE_MARK_FRAME			FrameMark;
// NOTE(Peter): Use VA_PROFILE_FUNC ONLY at the top of a function
//				Use VA_PROFILE_SCOPE / VA_PROFILE_SCOPE_DYNAMIC for an inner scope
#define VA_PROFILE_FUNC(...)			ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#define VA_PROFILE_SCOPE(...)			VA_PROFILE_FUNC(__VA_ARGS__)
#define VA_PROFILE_SCOPE_DYNAMIC(NAME)  ZoneScoped; ZoneName(NAME, strlen(NAME))
#define VA_PROFILE_THREAD(...)          tracy::SetThreadName(__VA_ARGS__)
#else
#define VA_PROFILE_MARK_FRAME
#define VA_PROFILE_FUNC(...)
#define VA_PROFILE_SCOPE(...)
#define VA_PROFILE_SCOPE_DYNAMIC(NAME)
#define VA_PROFILE_THREAD(...)
#endif