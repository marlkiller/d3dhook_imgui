#pragma once
#include <string>
#include <minwindef.h>


enum Enum
{
    None,

    D3D9,
    D3D10,
    D3D11,
    D3D12,

    OpenGL,
    Vulkan,

    Auto
};

namespace common_utils {
    void SearchModules();
    void getGameRect(HWND hwndGame, RECT& RectGame);
    int GetRandNumber(const int low, const int high);
    Enum GetDirectVersion();
    const char* enum_to_string(int type);
}
