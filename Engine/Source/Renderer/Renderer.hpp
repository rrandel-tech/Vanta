#pragma once

#include "RenderCommandQueue.hpp"
#include "RendererAPI.hpp"

#include <functional>

namespace Vanta {

    class Renderer
    {
    public:
        typedef void(*RenderCommandFn)(void*);

        // Commands
        static void Clear();
        static void Clear(float r, float g, float b, float a = 1.0f);
        static void SetClearColor(float r, float g, float b, float a);

    	static void DrawIndexed(uint32_t count);

        static void ClearMagenta();

        static void Init();

        static void* Submit(RenderCommandFn fn, uint32_t size)
        {
            return s_Instance->m_CommandQueue.Allocate(fn, size);
        }

        void WaitAndRender();

        inline static Renderer& Get() { return *s_Instance; }
    private:
        static Renderer* s_Instance;

        RenderCommandQueue m_CommandQueue;
    };

}

#define VA_RENDER_PASTE2(a, b) a ## b
#define VA_RENDER_PASTE(a, b) VA_RENDER_PASTE2(a, b)
#define VA_RENDER_UNIQUE(x) VA_RENDER_PASTE(x, __LINE__)

#define VA_RENDER(code) \
    struct VA_RENDER_UNIQUE(VARenderCommand) \
    {\
        static void Execute(void*)\
        {\
            code\
        }\
    };\
	{\
		auto mem = ::Vanta::Renderer::Submit(VA_RENDER_UNIQUE(VARenderCommand)::Execute, sizeof(VA_RENDER_UNIQUE(VARenderCommand)));\
		new (mem) VA_RENDER_UNIQUE(VARenderCommand)();\
	}\

#define VA_RENDER_1(arg0, code) \
	do {\
    struct VA_RENDER_UNIQUE(VARenderCommand) \
    {\
		VA_RENDER_UNIQUE(VARenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0) \
		: arg0(arg0) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg0;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
    };\
	{\
		auto mem = ::Vanta::Renderer::Submit(VA_RENDER_UNIQUE(VARenderCommand)::Execute, sizeof(VA_RENDER_UNIQUE(VARenderCommand)));\
		new (mem) VA_RENDER_UNIQUE(VARenderCommand)(arg0);\
	} } while(0)

#define VA_RENDER_2(arg0, arg1, code) \
    struct VA_RENDER_UNIQUE(VARenderCommand) \
    {\
		VA_RENDER_UNIQUE(VARenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1) \
		: arg0(arg0), arg1(arg1) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg1;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
    };\
	{\
		auto mem = ::Vanta::Renderer::Submit(VA_RENDER_UNIQUE(VARenderCommand)::Execute, sizeof(VA_RENDER_UNIQUE(VARenderCommand)));\
		new (mem) VA_RENDER_UNIQUE(VARenderCommand)(arg0, arg1);\
	}\

#define VA_RENDER_3(arg0, arg1, arg2, code) \
    struct VA_RENDER_UNIQUE(VARenderCommand) \
    {\
		VA_RENDER_UNIQUE(VARenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2) \
		: arg0(arg0), arg1(arg1), arg2(arg2) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg1;\
			auto& arg2 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg2;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2;\
    };\
	{\
		auto mem = ::Vanta::Renderer::Submit(VA_RENDER_UNIQUE(VARenderCommand)::Execute, sizeof(VA_RENDER_UNIQUE(VARenderCommand)));\
		new (mem) VA_RENDER_UNIQUE(VARenderCommand)(arg0, arg1, arg2);\
	}\

#define VA_RENDER_4(arg0, arg1, arg2, arg3, code) \
    struct VA_RENDER_UNIQUE(VARenderCommand) \
    {\
		VA_RENDER_UNIQUE(VARenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg3)>::type>::type arg3)\
		: arg0(arg0), arg1(arg1), arg2(arg2), arg3(arg3) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg1;\
			auto& arg2 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg2;\
			auto& arg3 = ((VA_RENDER_UNIQUE(VARenderCommand)*)argBuffer)->arg3;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg3)>::type>::type arg3;\
    };\
	{\
		auto mem = Renderer::Submit(VA_RENDER_UNIQUE(VARenderCommand)::Execute, sizeof(VA_RENDER_UNIQUE(VARenderCommand)));\
		new (mem) VA_RENDER_UNIQUE(VARenderCommand)(arg0, arg1, arg2, arg3);\
	}

#define VA_RENDER_S(code) auto self = this;\
	VA_RENDER_1(self, code)

#define VA_RENDER_S1(arg0, code) auto self = this;\
	VA_RENDER_2(self, arg0, code)

#define VA_RENDER_S2(arg0, arg1, code) auto self = this;\
	VA_RENDER_3(self, arg0, arg1, code)

#define VA_RENDER_S3(arg0, arg1, arg2, code) auto self = this;\
	VA_RENDER_4(self, arg0, arg1, arg2, code)