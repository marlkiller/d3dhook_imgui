
#include "d3d10_impl.h"
#include <d3d10.h>


#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx10.h"
#include "../logger.h"
#include "../detours.h"
#include "../imgui_constants.h"
#pragma comment(lib, "d3d10.lib")


typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

static Present oPresent = NULL;


static DWORD_PTR* pSwapChainVtable = NULL;
static DWORD_PTR* pContextVTable = NULL;
static DWORD_PTR* pDeviceVTable = NULL;

static ID3D10Device* pDevice = NULL;


long __stdcall hkPresent10(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool init = false;

	if (!init)
	{
		DXGI_SWAP_CHAIN_DESC desc;
		pSwapChain->GetDesc(&desc);
		ID3D10Device* device;
		pSwapChain->GetDevice(__uuidof(ID3D10Device), (void**)&device);

		//impl::win32::init(desc.OutputWindow);
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(desc.OutputWindow);
		ImGui_ImplDX10_Init(device);
		oWndProcHandler = (WNDPROC)SetWindowLongPtr(desc.OutputWindow, WNDPROC_INDEX, (LONG_PTR)hWndProc);
		LOG_INFO("Init with {%x},{%d},{%x},{%x}", desc.OutputWindow, WNDPROC_INDEX, (LONG_PTR)hWndProc, oWndProcHandler);
		init = true;
	}

	ImGui_ImplDX10_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//impl::showExampleWindow("D3D10");
	DrawGreetWin();
	DrawMainWin();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());

	return oPresent(pSwapChain, SyncInterval, Flags);
}

void impl::d3d10::init()
{

    HMODULE hDXGIDLL = 0;
    do
    {
        hDXGIDLL = GetModuleHandle("dxgi.dll");
        Sleep(4000);
        LOG_INFO("GetModuleHandle with dxgi.dll..");
    } while (!hDXGIDLL);
    Sleep(100);



    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
    RegisterClassExA(&wc);
    HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
    LOG_INFO("CreateWindowA >> hWnd {%d}", hWnd);

    D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    D3D_FEATURE_LEVEL obtainedLevel;

	IDXGISwapChain* pSwapChain;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_10_0;
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
	createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
	if (D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, D3D10_SDK_VERSION, &sd, &pSwapChain, &pDevice) != S_OK) {

		return;
		/*SwapChainVtable = *reinterpret_cast<void***>(pSwapChain);
		DeviceVtable = *reinterpret_cast<void***>(pDevice);
		for (unsigned char i = 0; i <= 97; i++)
			oDeviceFunc[i] = 0;
		for (unsigned char i = 0; i <= 17; i++)
			oSwapChainFunc[i] = 0;*/
	}
    
	pSwapChainVtable = (DWORD_PTR*)pSwapChain;
	pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

	/*pContextVTable = (DWORD_PTR*)pContext;
	pContextVTable = (DWORD_PTR*)pContextVTable[0];

	pDeviceVTable = (DWORD_PTR*)pDevice;
	pDeviceVTable = (DWORD_PTR*)pDeviceVTable[0];*/

	//oDrawIndexed = (D3D11DrawIndexedHook)(DWORD_PTR*)pContextVTable[12];
	oPresent = (Present)(DWORD_PTR*)pSwapChainVtable[8];

    //kiero::bind(8, (void**)&oPresent, hkPresent11);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(LPVOID&)oPresent, (PBYTE)hkPresent10);
    DetourTransactionCommit();
    LOG_INFO("DetourTransactionBegin hook complete >> oPresent {%x}, hkPresent{%x}", oPresent, hkPresent10);

    DWORD dwOld;
    VirtualProtect(oPresent, 2, PAGE_EXECUTE_READWRITE, &dwOld);

    LOG_INFO("VirtualProtect >> oPresent {%x}, dwOld{ %x}", oPresent, dwOld);

    /*while (true) {
        Sleep(10);
    }*/

    // Sleep(500);

    pDevice->Release();
    pSwapChain->Release();

    LOG_INFO("Device Release >> pDevice, pContext, pSwapChain");
}

