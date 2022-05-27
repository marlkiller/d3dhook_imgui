#include "pch.h"
#include "common_utils.h"
#include <iostream>
#include <Windows.h>
#include "logger.h"


namespace common_utils {
	

	int GetRandNumber(const int low, const int high)
	{
		int randNum = 0;
		randNum = rand() % (high - low + 1) + low;
		return randNum;
	}

	Enum GetDirectVersion();

	const char* enum_to_string(int index) {
		const char* enum_vals[] = { "None", "D3D9", "D3D10", "D3D11", "D3D12", "OpenGL", "Vulkan" };
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

