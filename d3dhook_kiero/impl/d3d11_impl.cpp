#include "../kiero.h"
#include "../global.h"

#if KIERO_INCLUDE_D3D11

#include "d3d11_impl.h"
#include <d3d11.h>
#include <assert.h>

#include "win32_impl.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"
#include <iostream>
#pragma comment(lib, "winmm.lib ")


typedef long(__stdcall* Present)(IDXGISwapChain*, UINT, UINT);
static Present oPresent = NULL;
static WNDPROC OriginalWndProcHandler = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


static bool p_open = false;
static bool greetings = false;

int un_init()
{
	kiero::unbind(8);
	kiero::shutdown();
	FreeLibrary(global::dllHWND);
	return 1;
}

LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGuiIO& io = ImGui::GetIO();
	//POINT mPos;
	//GetCursorPos(&mPos);
	//ScreenToClient(window, &mPos);
	//ImGui::GetIO().MousePos.x = mPos.x;
	//ImGui::GetIO().MousePos.y = mPos.y;

	if (uMsg == WM_KEYUP)
	{
		if (wParam == VK_INSERT)
		{
			p_open ^= 1;
			if (p_open)
				io.MouseDrawCursor = true;
			else
				io.MouseDrawCursor = false;
		}
	}

	if (p_open)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		return true;
	}

	return CallWindowProc(OriginalWndProcHandler, hWnd, uMsg, wParam, lParam);
}

static ID3D11DeviceContext* pContext = nullptr;
static ID3D11RenderTargetView* mainRenderTargetView = nullptr;

long __stdcall hkPresent11(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool init = false;
	if (!init)
	{
		std::cout << "init : %d" << init << std::endl;
		std::cout << "imgui first init" << std::endl;
		greetings = true;
		DXGI_SWAP_CHAIN_DESC desc;
		pSwapChain->GetDesc(&desc);
		ID3D11Device* pDevice;
		pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);

		pDevice->GetImmediateContext(&pContext);

		//impl::win32::init(desc.OutputWindow);
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(desc.OutputWindow);
		ImGui_ImplDX11_Init(pDevice,pContext);
		ID3D11Texture2D* pBackBuffer;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
		OriginalWndProcHandler = (WNDPROC)SetWindowLongPtr(desc.OutputWindow, GWLP_WNDPROC, (LONG_PTR)hWndProc);
		init = true;
	}
	

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO(); (void)io;	

	// greetings
	if (greetings)
	{
		std::cout << "greetings init" << std::endl;
		int sub_win_width = 300;
		int sub_win_height = 40;
		ImGui::SetNextWindowSize(ImVec2(sub_win_width, sub_win_height));
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2 - (sub_win_width / 2), io.DisplaySize.y / 2 - (sub_win_height / 2))); // 去掉 

		ImGui::Begin("title", &greetings, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
		ImGui::Text("Wallhack loaded, press INSERT for menu");
		ImGui::End();

		static DWORD lastTime = timeGetTime();
		DWORD timePassed = timeGetTime() - lastTime;
		if (timePassed > 6000)
		{
			std::cout << "greetings init timePassed : " << timePassed << std::endl;
			greetings = false;
			lastTime = timeGetTime();
		}
	}

	if (p_open)
		ImGui::GetIO().MouseDrawCursor = 1;
	else
		ImGui::GetIO().MouseDrawCursor = 0;

	std::cout << "p_open : " << p_open << std::endl;
	if (p_open) 
	{	
		ImGui::Begin("My Windows ", &p_open);
		ImGui::Text("Application average \n%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		if (ImGui::Button("Detach"))
		{
			ImGui::End();
			ImGui::EndFrame();
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)un_init, NULL, 0, NULL);
			return oPresent(pSwapChain, SyncInterval, Flags);
		}
		ImGui::End();
	}
	
	ImGui::EndFrame();
	ImGui::Render();
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}

void impl::d3d11::init()
{
	std::cout << "impl::d3d11::init" << std::endl;
	kiero::Status::Enum status = kiero::bind(8, (void**)&oPresent, hkPresent11);
	std::cout << "impl::d3d11::init : %d" << status << std::endl;
	//assert(kiero::bind(8, (void**)&oPresent, hkPresent11) == kiero::Status::Success);
}

#endif // KIERO_INCLUDE_D3D11