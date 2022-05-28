
#include "win32_impl.h"
#include <Windows.h>
#include <d3d11.h>

#include "../imgui_constants.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../logger.h"
#include "../common_utils.h"
#include "../imgui_draw_util.h"

static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
static ImVec4 clear_color = ImColor(114, 114, 114, 255).Value;
static RECT RectGame = { 100,100,100,100 };


void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

LRESULT WINAPI Win32WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (p_open)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        return true;
    }
    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void impl::win32::init()
{

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, Win32WndProc, 0L, 0L, Dll_HWND, NULL, NULL, NULL, NULL, "ImGui Example", NULL };
    ::RegisterClassEx(&wc);
    //HWND hwnd = ::CreateWindow(wc.lpszClassName, "Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 0, 0, 500, 500, NULL, NULL, wc.hInstance, NULL);

    //GAME_HWND = GetMainHWnd(GetCurrentThreadId());

    while (!GAME_HWND)
    {
        //GAME_HWND = GetMainHWnd(GetCurrentProcessId());
        GAME_HWND = GetHwndByPid(GetCurrentProcessId());
        //GAME_HWND = FindWindow(NULL, TEXT("无标题 - 记事本"));
        if (GAME_HWND)
            common_utils::getGameRect(GAME_HWND, RectGame);
        Sleep(500);
        LOG_INFO("Loading game win info");
    }

    LOG_INFO("GetCurrentProcessId {%d}, GAME_HWND {%x}, RectGame {%d,%d,%d,%d,}", GetCurrentProcessId(), GAME_HWND, RectGame.left, RectGame.top, RectGame.right - RectGame.left, RectGame.bottom - RectGame.top)

    HWND hwnd = ::CreateWindowEx(
        /*WS_EX_TOPMOST | WS_EX_TRANSPARENT|*/ WS_EX_LAYERED, // WS_EX_LAYERED will be alpha
        wc.lpszClassName, "Dear ImGui DirectX11 Example", 
        WS_POPUP/*WS_OVERLAPPEDWINDOW*/,  // WS_POPUP will hide title
        0, 0, 500, 500, 
        GAME_HWND, NULL, wc.hInstance, NULL);

    SetLayeredWindowAttributes(hwnd, RGB(114, 114, 114), NULL, LWA_COLORKEY); // //设置颜色过滤,使用改关键色刷新屏幕后颜色被过滤实现透明

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    ::SetWindowPos(hwnd, HWND_TOPMOST, RectGame.left, RectGame.top, RectGame.right - RectGame.left, RectGame.bottom - RectGame.top, SWP_SHOWWINDOW); // make win top

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        DrawGreetWin();
        DrawMainWin();

        if (GAME_HWND)
        {
            common_utils::getGameRect(GAME_HWND, RectGame); // auto move windows
            MoveWindow(hwnd, RectGame.left, RectGame.top, RectGame.right - RectGame.left, RectGame.bottom - RectGame.top, true);
        }
        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
}