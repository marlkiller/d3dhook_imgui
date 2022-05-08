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
    Enum GetDirectVersion();
}
