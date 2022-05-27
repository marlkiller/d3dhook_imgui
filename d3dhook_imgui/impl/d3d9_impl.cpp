
#include "d3d9_impl.h"
#include <d3d9.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../logger.h"
#include "../imgui_constants.h"
#include "../detours.h"

#pragma comment(lib, "d3d9.lib")
//#pragma comment(lib, "d3dx9.lib")

typedef long(__stdcall* Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
static Reset oReset = NULL;

typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
static EndScene oEndScene = NULL;

long __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	long result = oReset(pDevice, pPresentationParameters);
	ImGui_ImplDX9_CreateDeviceObjects();

	return result;
}

long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{

	if (!init)
	{
		D3DDEVICE_CREATION_PARAMETERS params;
		pDevice->GetCreationParameters(&params);
		oWndProcHandler = (WNDPROC)SetWindowLongPtr(params.hFocusWindow, WNDPROC_INDEX, (LONG_PTR)hWndProc);
		//impl::win32::init(params.hFocusWindow);
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(params.hFocusWindow);
		ImGui_ImplDX9_Init(pDevice);

		init = true;
		LOG_INFO("d3d9 init complate hwnd ,hFocusWindow {%d} ", params.hFocusWindow)
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	DrawGreetWin();
	DrawMainWin();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice);
}

void impl::d3d9::init()
{
	while (GetModuleHandle("d3d9.dll") == 0)
	{
		Sleep(100);
	}

	LOG_INFO("[d3d9] Finding for d3d9.dll ...{%x}", GetModuleHandle("d3d9.dll"));

	IDirect3D9* d3d = NULL;
	IDirect3DDevice9* pDevice = NULL;

	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);
	HWND tmpWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
	if (tmpWnd == NULL)
	{
		LOG_INFO("[d3d9] Failed to CreateWindowA");
		return;
	}

	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (d3d == NULL)
	{
		DestroyWindow(tmpWnd);
		LOG_INFO("[d3d9] Failed to Direct3DCreate9");
		return;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = tmpWnd;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	HRESULT result = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmpWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);
	if (result != D3D_OK)
	{
		d3d->Release();
		DestroyWindow(tmpWnd);
		LOG_INFO("[d3d9] Failed to CreateDevice");
		return;
	}
	LOG_INFO("[d3d9] CreateDevice D3D_OK");

	// We have the device, so walk the vtable to get the address of all the dx functions in d3d9.dll
#if defined _M_X64
	DWORD64* dVtable = (DWORD64*)pDevice;
	dVtable = (DWORD64*)dVtable[0];
#elif defined _M_IX86
	DWORD* dVtable = (DWORD*)pDevice;
	dVtable = (DWORD*)dVtable[0]; // == *d3ddev
#endif
	void** pVTable = *reinterpret_cast<void***>(pDevice);


	// Set EndScene_orig to the original EndScene etc.
	oEndScene = (EndScene)pVTable[42];
	//MH_CreateHook((DWORD_PTR*)dVtable[42], &hkEndScene, reinterpret_cast<void**>(&oEndScene)
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)oEndScene, /*(PBYTE)*/hkEndScene);
	DetourTransactionCommit();
	LOG_INFO("DetourTransactionBegin hook complete >> oEndScene {%x}-> hkEndScene {%x}", oEndScene, hkEndScene);

	//SetVertexShaderConstantF_orig = (SetVertexShaderConstantF)dVtable[94];
	//DrawIndexedPrimitive_orig = (DrawIndexedPrimitive)dVtable[82];
	//DrawPrimitive_orig = (DrawPrimitive)dVtable[81];
	//Reset_orig = (Reset)dVtable[16];
	//SetStreamSource_orig = (SetStreamSource)dVtable[100];
	////SetIndices_orig = (SetIndices)dVtable[104];
	//SetVertexDeclaration_orig = (SetVertexDeclaration)dVtable[87];
	//SetVertexShader_orig = (SetVertexShader)dVtable[92];
	//SetPixelShader_orig = (SetPixelShader)dVtable[107];
	//SetTexture_orig = (SetTexture)dVtable[65];
	//SetViewport_orig = (SetViewport)dVtable[47];

}