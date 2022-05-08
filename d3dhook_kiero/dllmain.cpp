// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <cstdio>

#include "kiero.h"
#include <Windows.h>
#include "global.h"
#include "logger.h"

using namespace std;


#if KIERO_INCLUDE_D3D9
# include "impl/d3d9_impl.h"
#endif

#if KIERO_INCLUDE_D3D10
# include "impl/d3d10_impl.h"
#endif

#if KIERO_INCLUDE_D3D11
# include "impl/d3d11_impl.h"
#endif
#include <iostream>

#if KIERO_INCLUDE_D3D12
#endif
#include "common_utils.h"

#if KIERO_INCLUDE_OPENGL
#endif

#if KIERO_INCLUDE_VULKAN
#endif

#if !KIERO_USE_MINHOOK
# error "The example requires that minhook be enabled!"
#endif

#ifdef _DEBUG
#define CONSOLE_LOGGING true
#else
#define CONSOLE_LOGGING false
#endif 

 
int mainThread()
{
   
    Enum version = common_utils::GetDirectVersion();

    LOG_INFO("Get directX version is {%d}",version);
    switch (version)
    {
        case None:
            break;
        case D3D9:
            //impl::d3d9::init();
            break;
        case D3D10:
            //impl::d3d10::init();
            break;
        case D3D11:
            impl::d3d11::init();
            break;
        case D3D12:
            break;
        case OpenGL:
            break;
        case Vulkan:
            break;
        case Auto:
            break;
        default:
            break;
    }
    return 0;
}
BOOL APIENTRY DllMain(HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //DisableThreadLibraryCalls(hModule);

        LOG_INFO("DLL_PROCESS_ATTACH {%d},{%d},{%d}", hModule, ul_reason_for_call, lpReserved)

        if (CONSOLE_LOGGING)
        {
            OPEN_COONSOLE();
        }
        LOG_INFO("Platform {%s},CONSOLE_LOGGING {%d}", IS_X64?"x64":"x86", CONSOLE_LOGGING);
        global::Dll_HWND = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainThread, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

