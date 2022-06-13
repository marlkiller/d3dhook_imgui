// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "dllmain.h"

#include <cstdio>

#include <Windows.h>
#include "imgui_constants.h"
#include "logger.h"
#include <iostream>
#include "common_utils.h"
#include "impl/d3d9_impl.h"
#include "impl/d3d10_impl.h"
#include "impl/d3d11_impl.h"
#include "impl/opengl_impl.h"
#include "impl/win32_impl.h"



using namespace std;

Enum version = OpenGL;

void SayHello()
{
    cout << "Hello DLL!" << endl;
}

//#if version==D3D9
//# include "impl/d3d9_impl.h"
//#endif
//#if  version==D3D10
//# include "impl/d3d10_impl.h"
//#endif
//#if  version==D3D11
//# include "impl/d3d11_impl.h"
//#endif
//#if KIERO_INCLUDE_D3D12
//#endif
//#if  version==OpenGL
//# include "impl/opengl_impl.h"
//#endif
//#if version==Vulkan
//#endif
//#if version==None
//# include "impl/win32_impl.h"
//#endif
 

#ifdef _DEBUG
#define CONSOLE_LOGGING true
#else
#define CONSOLE_LOGGING false
#endif 


int mainThread()
{
    //DisableThreadLibraryCalls(hModule);
    TCHAR full_path[MAX_PATH];
    GetModuleFileName(Dll_HWND, full_path, MAX_PATH);
    char fname[MAX_PATH] = { 0 };
    char ext[MAX_PATH] = { 0 };
    _splitpath(full_path, 0, 0, fname, ext);
    sprintf(MODULE_NAME, "%s%s", fname, ext);
    LOG_INFO("Support version is {None, D3D9, D3D10, D3D11, D3D12, OpenGL, Vulkan}");
    LOG_INFO("Current runtime path is {%s} - {%s}", full_path, MODULE_NAME);

    Enum pick_version = common_utils::string_to_enum(MODULE_NAME);
    if (pick_version != Auto)
        version = pick_version;
    LOG_INFO("Get directX version is {%d}-> %s", version, common_utils::enum_to_string(version));
    //Enum version = common_utils::GetDirectVersion();
    common_utils::SearchModules();

    switch (version)
    {
    case None:
        impl::win32::init();
        break;
    case D3D9:
        impl::d3d9::init();
        break;
    case D3D10:
        impl::d3d10::init();
        break;
    case D3D11:
        impl::d3d11::init();
        break;
    case D3D12:
        break;
    case OpenGL:
        impl::opengl::init();
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

        LOG_INFO("DLL_PROCESS_ATTACH {%d},{%d},{%d}", hModule, ul_reason_for_call, lpReserved);
        if (CONSOLE_LOGGING)
            OPEN_COONSOLE();
        LOG_INFO("Platform {%s},CONSOLE_LOGGING {%d}", IS_X64 ? "x64" : "x86", CONSOLE_LOGGING);
        Dll_HWND = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainThread, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

