## DirectX/OpenGL  detours hook. 

[opengl/d3d9/d3d10..??]Automatic adaptation is unstable,  

Sometimes you need to manually configure the target version in "mainThread"



### Build Environment
- DirectX SDK https://www.microsoft.com/en-us/download/details.aspx?id=6812
- OpenGL SDK
- c++/VisualStudio 
- ImGui/stb_image/detours

#### x64 ASM integration

Command line : ml64 /Fo $(IntDir)%(fileName).obj /c %(fileName).asm  

Output : $(IntDir)%(fileName).obj

![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/asm_64.jpg)
![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/call_msg_box.jpg)

### Preview

> Menu
![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/menu2.jpg)
![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/menu3.jpg)

> Win32
![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/win32.jpg)

> DirectX11
![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/dx11_1.jpg)
![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/dx11_2.jpg)

> OpenGL
![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/open_gl.jpg)

> LogWind
![example](https://raw.githubusercontent.com/marlkiller/d3dhook_kiero/master/image/log.jpg)

### TODO


## Supported Platform

### Windows
| DirectX                             | x86 | x64 |
| :---------------------------------- | :------: | :----: | 
| 9   |    ✅    |   ✅   | 
| 10  |    ✅    |   ✅   |
| 11  |    ✅    |   ✅   |  
| 12  |    ❌    |   ❌   |  

| OpenGL                             | x86 | x64 |
| :---------------------------------- | :------: | :----: | 
| 2   |    ✅    |   ❌   | 
| 3  |    ✅    |   ❌   | 


## Features:
- TODO
- 


## See Also:
- https://github.com/ocornut/imgui
- https://github.com/Rebzzel/kiero
- https://github.com/DarthTon/Xenos
