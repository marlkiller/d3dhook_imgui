#pragma once
#include <wtypes.h>

#if defined(_M_X64)	
# define WNDPROC_INDEX GWLP_WNDPROC
# define IS_X64 true
#else
# define WNDPROC_INDEX GWL_WNDPROC
# define IS_X64 false
#endif

namespace global
{
	extern HMODULE Dll_HWND;
	extern HWND GAME_HWND;
	
}
