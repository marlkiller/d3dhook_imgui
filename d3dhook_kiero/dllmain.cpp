// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <cstdio>

#include "kiero.h"
#include <Windows.h>
#include "console.h"
#include "global.h"

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

ConsoleUtil* Util = new ConsoleUtil();

int mainThread()
{
    if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
    {

        switch (kiero::getRenderType())
        {
#if KIERO_INCLUDE_D3D9
        case kiero::RenderType::D3D9:
            impl::d3d9::init();
            break;
#endif
#if KIERO_INCLUDE_D3D10
        case kiero::RenderType::D3D10:
            impl::d3d10::init();
            break;
#endif
#if KIERO_INCLUDE_D3D11
        case kiero::RenderType::D3D11:
            impl::d3d11::init();
            break;
#endif
        case kiero::RenderType::D3D12:
            // TODO: D3D12 implementation?
            break;
        case kiero::RenderType::OpenGL:
            // TODO: OpenGL implementation?
            break;
        case kiero::RenderType::Vulkan:
            // TODO: Vulkan implementation?
            break;
        }

        return 1;
    }
    else {
    }

    return 0;
}
BOOL APIENTRY DllMain(HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    //DisableThreadLibraryCalls(hModule);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (CONSOLE_LOGGING)
        {
            Util->OpenConsole();
            string log = "this is OpenConsole log";
            cout << "d3d hook >> " << log.c_str() << endl;
        }
        else {
            //Util->OpenConsole();
        }
        global::dllHWND = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainThread, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

