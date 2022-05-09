#include "../global.h"

#include "d3d11_impl.h"
#include <d3d11.h>
#include "win32_impl.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../logger.h"
#include "../imgui_plugin.h"
#include "../detours.h"
#include <exception>
#include <d3dcompiler.h>
#include <string>

#pragma comment(lib, "winmm.lib ")
#define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL; }
#define DEPTH_BIAS_D32_FLOAT(d) (d/(1/pow(2,23)))

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


typedef HRESULT(__stdcall* D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef void(__stdcall* D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

typedef void(__stdcall* DrawIndexed) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);



static DWORD_PTR* pSwapChainVtable = NULL;
static DWORD_PTR* pContextVTable = NULL;
static DWORD_PTR* pDeviceVTable = NULL;


static D3D11PresentHook oPresent = NULL;
static DrawIndexed oDrawIndexed = NULL;

static ID3D11Device* pDevice = NULL;
static ID3D11DeviceContext* pContext = nullptr;
static ID3D11RenderTargetView* mainRenderTargetView = nullptr;


static WNDPROC OriginalWndProcHandler = nullptr;

// wallhack
static ID3D11DepthStencilState* DepthStencilState_TRUE = NULL; //depth off
static ID3D11DepthStencilState* DepthStencilState_FALSE = NULL; //depth off
static ID3D11DepthStencilState* DepthStencilState_ORIG = NULL; //depth on

//shader
static ID3D11PixelShader* sGreen = NULL;
static ID3D11PixelShader* sMagenta = NULL;

//wh
ID3D11RasterizerState* DEPTHBIASState_FALSE;
ID3D11RasterizerState* DEPTHBIASState_TRUE;
ID3D11RasterizerState* DEPTHBIASState_ORIG;

static bool p_open = true;
static bool greetings = true;
static bool init = false;
static bool refresh_draw_items = false;

enum DrawItemColumnID
{
    // if (Stride == 56 && IndexCount == 912 && veWidth == 28672 && pscWidth == 592)
    DrawItemColumnID_ID,
    DrawItemColumnID_Stride,
    DrawItemColumnID_IndexCount,
    DrawItemColumnID_inWidth,
    DrawItemColumnID_Action,
    DrawItemColumnID_veWidth,
    DrawItemColumnID_pscWidth
};

struct DrawItem
{
    int         ID;
    int         Stride;
    int         IndexCount;
    int         veWidth;
    int         pscWidth;
    int         inWidth;
};

static ImVector<DrawItem> table_items;
static ImVector<int> selection;

static int id_number = 0;
static int draw_type = -1;


LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (p_open)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        return true;
    }

    return CallWindowProc(OriginalWndProcHandler, hWnd, uMsg, wParam, lParam);
}
HRESULT GenerateShader(ID3D11Device* pD3DDevice, ID3D11PixelShader** pShader, float r, float g, float b)
{
    char szCast[] = "struct VS_OUT"
        "{"
        "    float4 Position   : SV_Position;"
        "    float4 Color    : COLOR0;"
        "};"

        "float4 main( VS_OUT input ) : SV_Target"
        "{"
        "    float4 fake;"
        "    fake.a = 1.0;"
        "    fake.r = %f;"
        "    fake.g = %f;"
        "    fake.b = %f;"
        "    return fake;"
        "}";
    ID3D10Blob* pBlob;
    char szPixelShader[1000];

    sprintf(szPixelShader, szCast, r, g, b);

    HRESULT hr = D3DCompile(szPixelShader, sizeof(szPixelShader), "shader", NULL, NULL, "main", "ps_4_0", NULL, NULL, &pBlob, NULL);

    if (FAILED(hr))
        return hr;

    hr = pD3DDevice->CreatePixelShader((DWORD*)pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, pShader);

    if (FAILED(hr))
        return hr;

    return S_OK;
}



static int radio_stride = -1;
static int radio_inidex = -1;
static int radio_width = -1;
static int radio_psc_width = -1;
static int step_type = 1;

static DrawItem current_item;
static int current_count = -1;
const char* input_val = "";

void __stdcall hkDrawIndexed11(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    bool matched = false;
    //OUTPUT_DEBUG(L"hkDrawIndexed11 >> IndexCount %d ,StartIndexLocation %d ,BaseVertexLocation %d\n", IndexCount, StartIndexLocation, BaseVertexLocation);
    // change the stride TODO out log？
    if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x31) & 1))
    {
        radio_stride++;
    }
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x31) & 1))
    {
        radio_stride--;
    }
    // change the indexCount
    if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x32) & 1))
    {
        radio_inidex++;
    }
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x32) & 1))
    {
        radio_inidex--;
    }
    // change the veWidth
    if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x33) & 1))
    {
        radio_width++;
    }
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x33) & 1))
    {
        radio_width--;
    }
    // change the psc_width
    if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x34) & 1))
    {
        radio_psc_width++;
    }
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x34) & 1))
    {
        radio_psc_width--;
    }

    // change the current_item
    if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x30) & 1))
    {
        if (! (current_count + 1 >= table_items.size()))
        {
            current_count = current_count + 1;
            current_item = table_items[current_count];
            OUTPUT_DEBUG(L"current_count %d/%d --> ( %d,%d,%d,%d,%d)", table_items.size(), current_count + 1, current_item.Stride, current_item.IndexCount, current_item.inWidth, current_item.veWidth, current_item.pscWidth);

            char result_str[100];
            std::string result_s;
            sprintf(result_str, "%d: %d,%d,%d,%d", current_count + 1, current_item.Stride, current_item.IndexCount, current_item.veWidth, current_item.pscWidth);
            result_s = result_str;
            input_val = result_s.c_str();
        }
    }
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x30) & 1))
    {
        if (current_count>0)
        {
            current_count = current_count - 1;
            current_item = table_items[current_count];
            OUTPUT_DEBUG(L"current_count %d/%d --> ( %d,%d,%d,%d,%d)", table_items.size(), current_count + 1, current_item.Stride, current_item.IndexCount, current_item.inWidth, current_item.veWidth, current_item.pscWidth);

            char result_str[100];
            std::string result_s;
            sprintf(result_str, "%d: %d,%d,%d,%d", current_count + 1, current_item.Stride, current_item.IndexCount, current_item.veWidth, current_item.pscWidth);
            result_s = result_str;
            input_val = result_s.c_str();
        }
       
    }
    ID3D11Buffer* veBuffer;
    UINT veWidth;
    UINT Stride;
    UINT veBufferOffset;
    D3D11_BUFFER_DESC veDesc;

    //get models
    pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veBufferOffset);
    if (veBuffer) {
        veBuffer->GetDesc(&veDesc);
        veWidth = veDesc.ByteWidth;
    }
    if (NULL != veBuffer) {
        veBuffer->Release();
        veBuffer = NULL;
    }

    ID3D11Buffer* inBuffer;
    DXGI_FORMAT inFormat;
    UINT        inOffset;
    D3D11_BUFFER_DESC indesc;
    UINT inWidth; // remove??

    // get indesc.ByteWidth
    pContext->IAGetIndexBuffer(&inBuffer, &inFormat, &inOffset);
    if (inBuffer) {
        inBuffer->GetDesc(&indesc);
        inWidth = indesc.ByteWidth;
    }
        
    if (inBuffer != NULL) { inBuffer->Release(); inBuffer = NULL; }

    ID3D11Buffer* pscBuffer;
    UINT pscWidth;
    D3D11_BUFFER_DESC pscdesc;

    //get pscdesc.ByteWidth
    pContext->PSGetConstantBuffers(0, 1, &pscBuffer);
    if (pscBuffer) {
        pscBuffer->GetDesc(&pscdesc);
        pscWidth = pscdesc.ByteWidth;
    }
    if (NULL != pscBuffer) {
        pscBuffer->Release();
        pscBuffer = NULL;
    }


    if (refresh_draw_items)
    {
        bool exist = false;
        ImVector<DrawItem>::iterator it;
        for (it = table_items.begin(); it != table_items.end(); it++)
            if (it->Stride == Stride && it->IndexCount == IndexCount && it->veWidth == veWidth && it->pscWidth == pscWidth && it->inWidth == inWidth)
            {
                exist = true;
                continue;
            }
        if (!exist) {
            id_number = id_number + 1;
            DrawItem item;
            item.ID = id_number;
            item.Stride = Stride;
            item.inWidth = inWidth;
            item.IndexCount = IndexCount;
            item.veWidth = veWidth;
            item.pscWidth = pscWidth;
            table_items.push_back(item);

        }
    }

    if (current_count >= 0)
    {
        if (current_item.Stride == Stride && current_item.IndexCount == IndexCount && current_item.veWidth == veWidth && current_item.pscWidth == pscWidth && current_item.inWidth == inWidth)
        {
            matched = true;
        }
    }

    if ((draw_type != -1 && radio_stride == Stride))
    {
        // 1. >> first make target obj green , hide others
        //if (radio_stride == Stride) {

        matched = true;
        //2. << change the index, make obj if stil green
        if (radio_inidex > -1)
        {
            if (
                !(((step_type == 1) && ((radio_inidex == IndexCount / 10))) ||
                    ((step_type == 2) && (radio_inidex == IndexCount / 100)) ||
                    ((step_type == 3) && (radio_inidex == IndexCount / 1000)))
                ) {

                return oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
            }
            else
            {
                OUTPUT_DEBUG(L"2. filter radio_inidex >> ( [%d,>> %d],%d,%d) --> (IndexCount==%d && veWidth==%d && pscWidth==%d)||", Stride, IndexCount, veWidth, pscWidth);

                //3. << change the radio_width, make obj if stil green
                if (radio_width > -1)
                {
                    if (
                        !(((step_type == 1) && ((radio_width == veWidth / 100))) ||
                            ((step_type == 2) && (radio_width == veWidth / 1000)) ||
                            ((step_type == 3) && (radio_width == veWidth / 10000)))
                        ) {

                        return oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
                    }
                    else
                    {
                        OUTPUT_DEBUG(L"3. filter radio_width >> ([%d,%d,>> %d],%d)", Stride, IndexCount, veWidth, pscWidth);

                        //4. << change the radio_width, make obj if stil green
                        if (radio_psc_width > -1)
                        {
                            if (
                                !(((step_type == 1) && ((radio_psc_width == pscWidth / 1))) ||
                                    ((step_type == 2) && (radio_psc_width == pscWidth / 10)) ||
                                    ((step_type == 3) && (radio_psc_width == pscWidth / 100)))
                                ) {

                                return oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
                            }
                            else
                            {
                                /*if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x39) & 1))
                                {

                                    char result_str[100];
                                    std::string result_s;
                                    sprintf(result_str, "%d,%d,%d,%d", Stride, IndexCount, veWidth, pscWidth);
                                    result_s = result_str;
                                    input_val = result_s.c_str();
                                }*/

                                OUTPUT_DEBUG(L"4. filter radio_psc_width >> ([%d,%d,%d,>> %d])", Stride, IndexCount, veWidth, pscWidth);
                            }
                        }

                    }
                }
            }
        }
    }
    if (matched)
    {
        if (draw_type == 1 || draw_type == 0) {
            if (draw_type == 0 || draw_type == 1) 
            {
                //get orig
                pContext->OMGetDepthStencilState(&DepthStencilState_ORIG, 0); //get original
                //set off
                pContext->OMSetDepthStencilState(DepthStencilState_FALSE, 0); //depthstencil off

            }
            if (draw_type == 1) {
                pContext->PSSetShader(sGreen, NULL, NULL); // color1
            }
            oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw
            if (draw_type == 1) {
                pContext->PSSetShader(sMagenta, NULL, NULL);// color2
            }
            if (draw_type == 0 || draw_type == 1) {
                //restore orig
                pContext->OMSetDepthStencilState(DepthStencilState_ORIG, 0); //depthstencil on
                //release
                SAFE_RELEASE(DepthStencilState_ORIG); //release
            }

        }
        else if (draw_type == 2)
        {
            pContext->PSSetShader(sGreen, NULL, NULL);
            oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw
            return oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
        }
        else if (draw_type == 3)
        {
            return; //delete selected texture
        }

    }
    return oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}



long __stdcall hkPresent11(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{

    try {
        if (!init)
        {
            LOG_INFO("imgui first init {%d}", init)
                //greetings = true;
                DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            ID3D11Device* pDevice;
            pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);

            pDevice->GetImmediateContext(&pContext);

            //impl::win32::init(desc.OutputWindow);
            global::GAME_HWND = desc.OutputWindow;
            ImGui::CreateContext();
            ImGui_ImplWin32_Init(desc.OutputWindow);
            ImGui_ImplDX11_Init(pDevice, pContext);
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            OriginalWndProcHandler = (WNDPROC)SetWindowLongPtr(desc.OutputWindow, WNDPROC_INDEX, (LONG_PTR)hWndProc);
            LOG_INFO("Init with {%d},{%d},{%d},{%d}", desc.OutputWindow, WNDPROC_INDEX, (LONG_PTR)hWndProc, OriginalWndProcHandler);

            // Create depthstencil state
            D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
            depthStencilDesc.DepthEnable = TRUE;
            depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
            depthStencilDesc.StencilEnable = FALSE;
            depthStencilDesc.StencilReadMask = 0xFF;
            depthStencilDesc.StencilWriteMask = 0xFF;
            // Stencil operations if pixel is front-facing
            depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
            depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            // Stencil operations if pixel is back-facing
            depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
            depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            pDevice->CreateDepthStencilState(&depthStencilDesc, &DepthStencilState_FALSE);


            D3D11_RASTERIZER_DESC rasterizer_desc;
            ZeroMemory(&rasterizer_desc, sizeof(rasterizer_desc));
            rasterizer_desc.FillMode = D3D11_FILL_SOLID;
            rasterizer_desc.CullMode = D3D11_CULL_NONE; //D3D11_CULL_FRONT;
            rasterizer_desc.FrontCounterClockwise = false;
            float bias = 1000.0f;
            float bias_float = static_cast<float>(-bias);
            bias_float /= 10000.0f;
            rasterizer_desc.DepthBias = DEPTH_BIAS_D32_FLOAT(*(DWORD*)&bias_float);
            rasterizer_desc.SlopeScaledDepthBias = 0.0f;
            rasterizer_desc.DepthBiasClamp = 0.0f;
            rasterizer_desc.DepthClipEnable = true;
            rasterizer_desc.ScissorEnable = false;
            rasterizer_desc.MultisampleEnable = false;
            rasterizer_desc.AntialiasedLineEnable = false;
            pDevice->CreateRasterizerState(&rasterizer_desc, &DEPTHBIASState_FALSE);


            if (!sGreen) {
                GenerateShader(pDevice, &sGreen, 0.0f, 1.0f, 0.0f); //green
            }


            if (!sMagenta) {
                GenerateShader(pDevice, &sMagenta, 1.0f, 0.0f, 1.0f); //magenta
            }
            /*DrawItem item;
            item.Stride = 1;
            item.IndexCount = 2;
            item.veWidth = 3;
            item.pscWidth= 4;
            table_items.push_back(item);*/


            init = true;
        }


        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        static bool draw_fov = false;
        static bool draw_filled_fov = false;
        static int fov_size = 0;
        static float bg_alpha = 1;
        static char input_buffer[64] = "";

        // greetings
        if (greetings)
        {
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            float sub_win_width = 300;
            float sub_win_height = 40;
            ImGui::SetNextWindowSize(ImVec2(sub_win_width, sub_win_height));
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2 - (sub_win_width / 2), io.DisplaySize.y / 2 - (sub_win_height / 2)));

            ImGui::Begin("title", &greetings, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
            ImGui::Text("Plugin loaded, press INSERT for menu");
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

        if (GetAsyncKeyState(VK_INSERT) & 1) p_open = !p_open;
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.MouseDrawCursor = p_open;

        if (p_open)
        {
            ImGui::SetNextWindowBgAlpha(bg_alpha);
            ImGui::Begin("My Windows ");
            ImGui::Text("Application average \n%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            if (ImGui::Checkbox("DrawFOV", &draw_fov))
            {
                if (draw_fov && !fov_size)
                {
                    fov_size = 100;
                }
            }
            ImGui::Checkbox("DrawFilledFOV", &draw_filled_fov);

            ImGui::SliderInt("fov_size", &fov_size, 0, 200, "fov_size:%d");
            ImGui::SliderFloat("bg_alpha", &bg_alpha, 0.0f, 1.0f, "bg_alpha:%.1f");

            ImGui::RadioButton("None", &draw_type, -1); ImGui::SameLine();
            ImGui::RadioButton("draw_Z", &draw_type, 0); ImGui::SameLine();
            ImGui::RadioButton("draw_Z&draw_color", &draw_type, 1); ImGui::SameLine();
            ImGui::RadioButton("draw_color", &draw_type, 2); ImGui::SameLine();
            ImGui::RadioButton("draw_hide", &draw_type, 3);

            ImGui::NewLine();
            ImGui::SliderInt("Step Mode", &step_type, 1, 3);
            if (step_type == 1) 
            {
                ImGui::Text("Mode 1 (IndexCount/10 & veWidth/100 & pscWidth/1)");
            }
            else if (step_type == 2)
            {
                ImGui::Text("Mode 1 (IndexCount/100 & veWidth/1000 & pscWidth/10)");
            }
            else if (step_type == 3)
            {
                ImGui::Text("Mode 1 (IndexCount/1000 & veWidth/10000 & pscWidth/100)");
            }

            ImGui::SliderInt("Stride", &radio_stride, -1, 100, "Stride:%d");ImGui::SameLine();ImGui::Text("Alt + 1 (add)| Ctrl + 1 (min)");
            ImGui::SliderInt("IndexCount", &radio_inidex, -1, 100, "IndexCount:%d");ImGui::SameLine();ImGui::Text("Alt + 2 (add)| Ctrl + 2 (min)");
            ImGui::SliderInt("veWidth", &radio_width, -1, 100, "veWidth:%d");ImGui::SameLine();ImGui::Text("Alt + 3 (add)| Ctrl + 3 (min)");
            ImGui::SliderInt("pscWidth", &radio_psc_width, -1, 100, "pscWidth:%d");ImGui::SameLine();ImGui::Text("Alt + 4 (add)| Ctrl + 4 (min)");

            if (ImGui::Button("Detach"))
            {
                ImGui::End();
                ImGui::EndFrame();
                ImGui::Render();
                pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

                ImGui_ImplDX11_Shutdown();
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();

                SetWindowLongPtr(global::GAME_HWND, WNDPROC_INDEX, (LONG_PTR)OriginalWndProcHandler);
                LOG_INFO("Detach with {%d},{%d},{%d}", global::GAME_HWND, WNDPROC_INDEX, OriginalWndProcHandler);
                {
                    try
                    {
                        DetourTransactionBegin();
                        DetourUpdateThread(GetCurrentThread());
                        DetourDetach(&(LPVOID&)oPresent, (PBYTE)hkPresent11);
                        DetourDetach(&(LPVOID&)oDrawIndexed, (PBYTE)hkDrawIndexed11);
                        DetourTransactionCommit();
                        FreeLibrary(global::Dll_HWND);

                    }
                    catch (...)
                    {
                        std::exception_ptr p = std::current_exception();
                        LOG_ERROR("ERROR");
                        LOG_ERROR("ERROR : {%s}", p);
                        throw;
                    }
                }
                return oPresent(pSwapChain, SyncInterval, Flags);
            }

            ImGui::Checkbox("Refresh Draw Data", &refresh_draw_items);

            
            static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
            ImGui::Text(input_val); ImGui::SameLine();
            if (ImGui::Button("ResetIndex"))
            {
                current_count = -1;

            }
            ImGui::SameLine();
            if (ImGui::Button("Copy"))
            {
                if (::OpenClipboard(NULL))
                {
                    ::EmptyClipboard();
                    HGLOBAL clipbuffer;
                    char* buffer;
                    clipbuffer = ::GlobalAlloc(GMEM_DDESHARE, strlen(input_val) + 1);
                    buffer = (char*)::GlobalLock(clipbuffer);
                    strcpy(buffer, input_val);
                    ::GlobalUnlock(clipbuffer);
                    ::SetClipboardData(CF_TEXT, clipbuffer);
                    ::CloseClipboard();
                }

            }

            // Update item list if we changed the number of items， TODO remove if ？
            if (ImGui::BeginTable("table_advanced", 8, flags, ImVec2(0, 0), 0.0f))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, DrawItemColumnID_ID);
                ImGui::TableSetupColumn("Stride", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, DrawItemColumnID_Stride);
                ImGui::TableSetupColumn("IndexCount", ImGuiTableColumnFlags_WidthFixed, 0.0f, DrawItemColumnID_IndexCount);
                ImGui::TableSetupColumn("inWidth", ImGuiTableColumnFlags_WidthFixed, 0.0f, DrawItemColumnID_inWidth);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, DrawItemColumnID_Action);
                ImGui::TableSetupColumn("veWidth", ImGuiTableColumnFlags_NoSort, 0.0f, DrawItemColumnID_veWidth);
                ImGui::TableSetupColumn("pscWidth", ImGuiTableColumnFlags_NoSort, 0.0f, DrawItemColumnID_pscWidth);
                ImGui::TableSetupColumn("Hidden", ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoSort);
                ImGui::TableSetupScrollFreeze(1, 1);
                ImGui::TableHeadersRow();
                ImGui::PushButtonRepeat(true);
                
                // Demonstrate using clipper for large vertical lists
                ImGuiListClipper clipper;
                clipper.Begin(table_items.Size);
                while (clipper.Step())
                {
                    for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                    {
                        DrawItem* item = &table_items[row_n];
                        //if (!filter.PassFilter(item->Name))
                        //    continue;
                        const bool item_is_selected = selection.contains(item->ID);
                        ImGui::PushID(item->ID);
                        ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

                        // For the demo purpose we can select among different type of items submitted in the first column
                        ImGui::TableSetColumnIndex(0);
                        char label[32];
                        sprintf(label, "%d", item->ID);
                        ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                        if (ImGui::Selectable(label, item_is_selected, selectable_flags, ImVec2(0, 0)))
                        {
                            selection.clear();
                            selection.push_back(item->ID);
                        }

                        if (ImGui::TableSetColumnIndex(1))
                            ImGui::Text("%d", item->Stride);

                        if (ImGui::TableSetColumnIndex(2))
                            ImGui::Text("%d", item->IndexCount);

                        if (ImGui::TableSetColumnIndex(3))
                            ImGui::Text("%d", item->inWidth);

                        if (ImGui::TableSetColumnIndex(4))
                        {
                            if (ImGui::SmallButton("SELECT")) {}
                            ImGui::SameLine();
                            if (ImGui::SmallButton("CLEAN")) {}
                        }

                        if (ImGui::TableSetColumnIndex(5))
                            ImGui::Text("%d", item->veWidth);

                        if (ImGui::TableSetColumnIndex(6))
                            ImGui::Text("%d", item->pscWidth);

                        if (ImGui::TableSetColumnIndex(7))
                            ImGui::Text("");

                        ImGui::PopID();
                    }
                }
                ImGui::PopButtonRepeat();
                ImGui::EndTable();
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

        //OUTPUT_DEBUG(L"init {%d},greetings {%d},p_open {%d}, draw_fov {%d},draw_filled_fov {%d}\n", init, greetings, p_open, draw_fov, draw_filled_fov);
        }
    catch (...) {
        std::exception_ptr p = std::current_exception();
        LOG_ERROR("ERROR");
        LOG_ERROR("ERROR : {%s}", p);
        throw;
    }
    return oPresent(pSwapChain, SyncInterval, Flags);
}




void impl::d3d11::init()
{

    HMODULE hDXGIDLL = 0;
    do
    {
        hDXGIDLL = GetModuleHandle("dxgi.dll");
        Sleep(4000);
    } while (!hDXGIDLL);
    Sleep(100);


    IDXGISwapChain* pSwapChain;

    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
    RegisterClassExA(&wc);
    HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

    D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    D3D_FEATURE_LEVEL obtainedLevel;
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;

    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(scd));
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Windowed = ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

    // LibOVR 0.4.3 requires that the width and height for the backbuffer is set even if
    // you use windowed mode, despite being optional according to the D3D11 documentation.
    scd.BufferDesc.Width = 1;
    scd.BufferDesc.Height = 1;
    scd.BufferDesc.RefreshRate.Numerator = 0;
    scd.BufferDesc.RefreshRate.Denominator = 1;

    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        requestedLevels,
        sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL),
        D3D11_SDK_VERSION,
        &scd,
        &pSwapChain,
        &pDevice,
        &obtainedLevel,
        &pContext)))
    {
        MessageBox(hWnd, "Failed to create directX device and swapchain!", "Error", MB_ICONERROR);
        return;
    }


    pSwapChainVtable = (DWORD_PTR*)pSwapChain;
    pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

    pContextVTable = (DWORD_PTR*)pContext;
    pContextVTable = (DWORD_PTR*)pContextVTable[0];

    pDeviceVTable = (DWORD_PTR*)pDevice;
    pDeviceVTable = (DWORD_PTR*)pDeviceVTable[0];

    oDrawIndexed = (D3D11DrawIndexedHook)(DWORD_PTR*)pContextVTable[12];
    oPresent = (D3D11PresentHook)(DWORD_PTR*)pSwapChainVtable[8];



    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(LPVOID&)oPresent, (PBYTE)hkPresent11);
    DetourAttach(&(LPVOID&)oDrawIndexed, (PBYTE)hkDrawIndexed11);
    DetourTransactionCommit();

    DWORD dwOld;
    VirtualProtect(oPresent, 2, PAGE_EXECUTE_READWRITE, &dwOld);

    /*while (true) {
        Sleep(10);
    }*/

    Sleep(5000);

    pDevice->Release();
    pContext->Release();
    pSwapChain->Release();

}


