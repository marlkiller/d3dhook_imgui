#include "pch.h"
#include "common_utils.h"
#include <iostream>
#include <Windows.h>


namespace common_utils {
	Enum GetDirectVersion();


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

