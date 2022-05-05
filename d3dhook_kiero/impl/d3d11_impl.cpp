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
#include "../logger.h"
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


static bool draw_fov = false;
static bool draw_filled_fov = false;
static int fov_size = 0;

long __stdcall hkPresent11(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool init = false;
	if (!init)
	{
		LOG_INFO("imgui first init {%d}", init)
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
			LOG_INFO("greetings init timePassed {%d}", timePassed);
			greetings = false;
			lastTime = timeGetTime();
		}
	}

	if (p_open)
		ImGui::GetIO().MouseDrawCursor = 1;
	else
		ImGui::GetIO().MouseDrawCursor = 0;

 	if (p_open) 
	{	
		ImGui::Begin("My Windows ", &p_open);
		ImGui::Text("Application average \n%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		if (ImGui::Checkbox("DrawFOV", &draw_fov))
		{
			if (draw_fov && !fov_size)
			{
				fov_size = 100;
			}
		}
		ImGui::Checkbox("DrawFilledFOV", &draw_filled_fov);

		ImGui::SliderInt("fov_size", &fov_size, 0, 200, "fov_size:%d%%");

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

	const auto draw = ImGui::GetBackgroundDrawList();
	static const auto size = ImGui::GetIO().DisplaySize;
	static const auto center = ImVec2(size.x / 2, size.y / 2);

	if (draw_fov)
		draw->AddCircle(center, fov_size, ImColor(255, 255, 255), 100);

	if (draw_filled_fov)
		draw->AddCircleFilled(center, fov_size, ImColor(0, 0, 0, 140), 100);
	
	ImGui::EndFrame();
	ImGui::Render();
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}

void impl::d3d11::init()
{
	kiero::Status::Enum status = kiero::bind(8, (void**)&oPresent, hkPresent11);
	LOG_INFO("impl::d3d11::init {%d}", status);
}

#endif // KIERO_INCLUDE_D3D11