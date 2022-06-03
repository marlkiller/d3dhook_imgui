#include "pch.h"
#include "common_utils.h"
#include <iostream>
#include <Windows.h>
#include "logger.h"


namespace common_utils {
	

	void getGameRect(HWND hwndGame, RECT& RectGame) {
		RECT stRect, stKhRect;
		GetWindowRect(hwndGame, &stRect);
		GetClientRect(hwndGame, &stKhRect);
		RectGame.left = stRect.left;
		RectGame.right = stRect.right;
		RectGame.top = stRect.bottom - stKhRect.bottom;
		RectGame.bottom = stRect.bottom;
	}
	int GetRandNumber(const int low, const int high)
	{
		int randNum = 0;
		randNum = rand() % (high - low + 1) + low;
		return randNum;
	}

	void SearchModules() {
		HMODULE handler;
		handler = GetModuleHandle("opengl32.dll");
		LOG_INFO("opengl32.dll (%x) -> {%s} ", handler, handler ? "OK" : "NO");
		handler = GetModuleHandle("vulkan-1.dll");
		LOG_INFO("vulkan-1.dll (%x) -> {%s} ", handler, handler ? "OK" : "NO");
		handler = GetModuleHandle("d3d9.dll");
		LOG_INFO("d3d9.dll (%x) -> {%s} ", handler, handler ? "OK" : "NO");
		handler = GetModuleHandle("d3d10.dll");
		LOG_INFO("d3d10.dll (%x) -> {%s} ", handler, handler ? "OK" : "NO");
		handler = GetModuleHandle("d3d11.dll");
		LOG_INFO("d3d11.dll (%x) -> {%s} ", handler, handler ? "OK" : "NO");
		handler = GetModuleHandle("d3d12.dll");
		LOG_INFO("d3d12.dll (%x) -> {%s} ", handler, handler ? "OK" : "NO");
		
		
		
	}

	const char* enum_vals[] = { "None", "D3D9", "D3D10", "D3D11", "D3D12", "OpenGL", "Vulkan" };
	const Enum enum_keys[] = { None,D3D9, D3D10,D3D11,D3D12,OpenGL,Vulkan};


	Enum string_to_enum(char* val) {

		for (int i = 0; i < 7; i++)
		{
			if (strstr(val, enum_vals[i]))
				return enum_keys[i];
		}
		return Auto;
	}
	const char* enum_to_string(int index) {
		return  enum_vals[index];
	}


	Enum GetDirectVersion() {

		Enum type = None;

		if (::GetModuleHandle(("d3d12.dll")) != NULL)
		{
			type = D3D12;
		}
		else if (::GetModuleHandle(("d3d11.dll")) != NULL)
		{
			type = D3D11;
		}
		else if (::GetModuleHandle(("d3d10.dll")) != NULL)
		{
			type = D3D10;
		}
		else if (::GetModuleHandle(("d3d9.dll")) != NULL)
		{
			type = D3D9;
		}
		else if (::GetModuleHandle(("opengl32.dll")) != NULL)
		{
			type = OpenGL;
		}
		else if (::GetModuleHandle(("vulkan-1.dll")) != NULL)
		{
			type = Vulkan;
		}
		return type;
	};
}

