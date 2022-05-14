﻿#include "../global.h"

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
#include <DirectXMath.h>
#include <vector>
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


//wh
UINT stencilRef = 0;
D3D11_DEPTH_STENCIL_DESC Desc;
ID3D11DepthStencilState* oDepthStencilState = NULL;
ID3D11DepthStencilState* pDepthStencilState = NULL;
ID3D11DepthStencilState* depthStencilStateFalse;


//shader
static ID3D11PixelShader* sGreen = NULL;
static ID3D11PixelShader* sMagenta = NULL;

static ImColor color_red = ImColor(255, 0, 0, 255);


//wh
ID3D11RasterizerState* DEPTHBIASState_FALSE;
ID3D11RasterizerState* DEPTHBIASState_TRUE;
ID3D11RasterizerState* DEPTHBIASState_ORIG;

static bool p_open = false;
static bool greetings = true;
static bool init = false;
static bool refresh_draw_items = false;

static int draw1_x = -1;
static int draw1_y = -1;

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
    int         Stride = 0;
    int         IndexCount = 0;
    int         veWidth = 0;
    int         pscWidth = 0;
    int         inWidth = 0;
};

static ImVector<DrawItem> table_items;
static ImVector<int> selection;

static int id_number = 0;
static int draw_type = 0;


LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //ImGuiIO& io = ImGui::GetIO();
    /*POINT mPos;
    GetCursorPos(&mPos);
    ScreenToClient(global::GAME_HWND, &mPos);
    ImGui::GetIO().MousePos.x = mPos.x;
    ImGui::GetIO().MousePos.y = mPos.y;*/

    /*if (uMsg == WM_KEYUP)
    {
        if (wParam == VK_INSERT)
        {
            p_open = !p_open;
        }
    }*/

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

static int find_model_type = 0;
static DrawItem current_item;
static int current_count = -1;
int aimheight = 0;
//Viewport
float ViewportWidth;
float ViewportHeight;
float ScreenCenterX;
float ScreenCenterY;

//w2s
int WorldViewCBnum = 2; //2
int ProjCBnum = 1;//1
int matProjnum = 16;//16

ID3D11Buffer* pWorldViewCB = nullptr;
ID3D11Buffer* pProjCB = nullptr;

ID3D11Buffer* m_pCurWorldViewCB = NULL;
ID3D11Buffer* m_pCurProjCB = NULL;

float matWorldView[4][4];
float matProj[4][4];

float* worldview;
float* proj;
ID3D11Buffer* pStageBufferA = NULL;
ID3D11Buffer* pStageBufferB = NULL;

ID3D11SamplerState* pSamplerState;
ID3D11Texture2D* textureRed = nullptr;
ID3D11Texture2D* textureGreen = nullptr;

ID3D11ShaderResourceView* textureViewRed;
ID3D11ShaderResourceView* textureViewGreen;


void MapBuffer(ID3D11Buffer* pStageBuffer, void** ppData, UINT* pByteWidth)
{
    D3D11_MAPPED_SUBRESOURCE subRes;
    HRESULT res = pContext->Map(pStageBuffer, 0, D3D11_MAP_READ, 0, &subRes);
    OUTPUT_DEBUG(L"pContext->Map(pStageBuffer, 0, D3D11_MAP_READ, 0, &subRes)");
    D3D11_BUFFER_DESC desc;
    pStageBuffer->GetDesc(&desc);

    if (FAILED(res))
    {
        OUTPUT_DEBUG(L"Map stage buffer failed {%d} {%d} {%d} {%d} {%d}", (void*)pStageBuffer, desc.ByteWidth, desc.BindFlags, desc.CPUAccessFlags, desc.Usage);
        SAFE_RELEASE(pStageBuffer); 
        return;
    }

    *ppData = subRes.pData;

    if (pByteWidth)
        *pByteWidth = desc.ByteWidth;
}

void UnmapBuffer(ID3D11Buffer* pStageBuffer)
{
    pContext->Unmap(pStageBuffer, 0);
}
ID3D11Buffer* CopyBufferToCpuB(ID3D11Buffer* pBufferB)
{
    D3D11_BUFFER_DESC CBDescB;
    pBufferB->GetDesc(&CBDescB);

    if (pStageBufferB == NULL)
    {
        //Log("onceB");
        // create buffer
        D3D11_BUFFER_DESC descB;
        descB.BindFlags = 0;
        descB.ByteWidth = CBDescB.ByteWidth;
        descB.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        descB.MiscFlags = 0;
        descB.StructureByteStride = 0;
        descB.Usage = D3D11_USAGE_STAGING;

        if (FAILED(pDevice->CreateBuffer(&descB, NULL, &pStageBufferB)))
        {
            //Log("CreateBuffer failed when CopyBufferToCpuB {}");
        }
    }

    if (pStageBufferB != NULL)
        pContext->CopyResource(pStageBufferB, pBufferB);

    return pStageBufferB;
}
float GetDst(float Xx, float Yy, float xX, float yY)
{
    return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}
ID3D11Buffer* CopyBufferToCpuA(ID3D11Buffer* pBufferA)
{
    D3D11_BUFFER_DESC CBDescA;
    pBufferA->GetDesc(&CBDescA);

    if (pStageBufferA == NULL)
    {
        //Log("onceA");
        // create buffer
        D3D11_BUFFER_DESC descA;
        descA.BindFlags = 0;
        descA.ByteWidth = CBDescA.ByteWidth;
        descA.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        descA.MiscFlags = 0;
        descA.StructureByteStride = 0;
        descA.Usage = D3D11_USAGE_STAGING;

        if (FAILED(pDevice->CreateBuffer(&descA, NULL, &pStageBufferA)))
        {
            //Log("CreateBuffer failed when CopyBufferToCpuA {}");
        }
    }

    if (pStageBufferA != NULL)
        pContext->CopyResource(pStageBufferA, pBufferA);

    return pStageBufferA;
}

void AddModel(ID3D11DeviceContext* pContext)
{


    pContext->VSGetConstantBuffers(WorldViewCBnum, 1, &pWorldViewCB);	//WorldViewCBnum
    pContext->VSGetConstantBuffers(ProjCBnum, 1, &pProjCB);
    if (pWorldViewCB == nullptr)
    {
        OUTPUT_DEBUG(L"pWorldViewCB == nullptr");
        SAFE_RELEASE(pWorldViewCB); return;
    }
    if (pProjCB == nullptr)
    {
        OUTPUT_DEBUG(L"pProjCB == nullptr");
        SAFE_RELEASE(pProjCB); return;
    }
    if (pWorldViewCB != nullptr && pProjCB != nullptr)
    {
        m_pCurWorldViewCB = CopyBufferToCpuA(pWorldViewCB);
        m_pCurProjCB = CopyBufferToCpuB(pProjCB);
    }
    if (m_pCurWorldViewCB == nullptr)
    {
        OUTPUT_DEBUG(L"m_pCurWorldViewCB == nullptr");
        SAFE_RELEASE(m_pCurWorldViewCB); return;
    }
    if (m_pCurProjCB == nullptr)
    {
        OUTPUT_DEBUG(L"m_pCurProjCB == nullptr");
        SAFE_RELEASE(m_pCurProjCB); return;
    }

    if (m_pCurWorldViewCB != nullptr && m_pCurProjCB != nullptr)
    {
        OUTPUT_DEBUG(L"m_pCurWorldViewCB != nullptr && m_pCurProjCB != nullptr");
        MapBuffer(m_pCurWorldViewCB, (void**)&worldview, NULL);
        memcpy(matWorldView, &worldview[0], sizeof(matWorldView));
        UnmapBuffer(m_pCurWorldViewCB);
        MapBuffer(m_pCurProjCB, (void**)&proj, NULL);
        memcpy(matProj, &proj[matProjnum], sizeof(matProj));			//matProjnum
        UnmapBuffer(m_pCurProjCB);
    }

    OUTPUT_DEBUG(L"int method1 = 4");
    int method1 = 4;
    if (method1 == 1)
    {
        DirectX::XMVECTOR Pos = DirectX::XMVectorSet(0.0f, 0.0f, aimheight, 1.0f);
        DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply((DirectX::FXMMATRIX)*matWorldView, (DirectX::FXMMATRIX)*matProj);//multipication order matters

        //normal
        DirectX::XMMATRIX WorldViewProj = viewProjMatrix; //normal

        float mx = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[0] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[0] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[0] + WorldViewProj.r[3].m128_f32[0];
        float my = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[1] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[1] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[1] + WorldViewProj.r[3].m128_f32[1];
        float mz = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[2] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[2] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[2] + WorldViewProj.r[3].m128_f32[2];
        float mw = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[3] + WorldViewProj.r[3].m128_f32[3];

        float xx, yy;
        xx = ((mx / mw) * (ViewportWidth / 2.0f)) + (ViewportWidth / 2.0f);
        yy = (ViewportHeight / 2.0f) - ((my / mw) * (ViewportHeight / 2.0f)); //- or + depends on the game

        OUTPUT_DEBUG(L"draw1 %d,%d,%d", xx, yy, mw);
        draw1_x = xx;
        draw1_y = yy;

    }
    if (method1 == 2)
    {
        DirectX::XMVECTOR Pos = DirectX::XMVectorSet(0.0f, 0.0f, aimheight, 1.0f);
        DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply((DirectX::FXMMATRIX)*matWorldView, (DirectX::FXMMATRIX)*matProj);//multipication order matters

        //transpose
        DirectX::XMMATRIX WorldViewProj = DirectX::XMMatrixTranspose(viewProjMatrix); //transpose

        float mx = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[0] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[0] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[0] + WorldViewProj.r[3].m128_f32[0];
        float my = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[1] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[1] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[1] + WorldViewProj.r[3].m128_f32[1];
        float mz = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[2] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[2] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[2] + WorldViewProj.r[3].m128_f32[2];
        float mw = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[3] + WorldViewProj.r[3].m128_f32[3];

        float xx, yy;
        xx = ((mx / mw) * (ViewportWidth / 2.0f)) + (ViewportWidth / 2.0f);
        yy = (ViewportHeight / 2.0f) - ((my / mw) * (ViewportHeight / 2.0f)); //- or + depends on the game

        OUTPUT_DEBUG(L"draw2 %d,%d,%d", xx, yy, mw);
        draw1_x = xx;
        draw1_y = yy;
    }

  
    if (method1 == 3)
    {
        //TAB worldtoscreen unity 2018
        DirectX::XMVECTOR Pos = DirectX::XMVectorSet(0.0f, aimheight, 0.0f, 1.0);
        //DirectX::XMVECTOR Pos = XMVectorSet(0.0f, aimheight, preaim, 1.0);
        DirectX::XMMATRIX viewProjMatrix = DirectX::XMMatrixMultiply((DirectX::FXMMATRIX)*matWorldView, (DirectX::FXMMATRIX)*matProj);//multipication order matters

        //normal
        DirectX::XMMATRIX WorldViewProj = viewProjMatrix; //normal

        float mx = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[0] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[0] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[0] + WorldViewProj.r[3].m128_f32[0];
        float my = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[1] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[1] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[1] + WorldViewProj.r[3].m128_f32[1];
        float mz = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[2] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[2] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[2] + WorldViewProj.r[3].m128_f32[2];
        float mw = Pos.m128_f32[0] * WorldViewProj.r[0].m128_f32[3] + Pos.m128_f32[1] * WorldViewProj.r[1].m128_f32[3] + Pos.m128_f32[2] * WorldViewProj.r[2].m128_f32[3] + WorldViewProj.r[3].m128_f32[3];

        if (mw > 1.0f)
        {
            float invw = 1.0f / mw;
            mx *= invw;
            my *= invw;

            float x = ViewportWidth / 2.0f;
            float y = ViewportHeight / 2.0f;

            x += 0.5f * mx * ViewportWidth + 0.5f;
            y += 0.5f * my * ViewportHeight + 0.5f;//-
            mx = x;
            my = y;
        }
        else
        {
            mx = -1.0f;
            my = -1.0f;
        }

        OUTPUT_DEBUG(L"draw3 %d,%d,%d", mx, my, mw);
        draw1_x = mx;
        draw1_y = my;
    }

    if (method1 == 4)
    {
        
        //new unity incomplete, todo: fix matrix is flipped
        //1, 2, 43
        /*D3DXMATRIX matrix, m1;
        D3DXVECTOR4 position;
        D3DXVECTOR4 input;
        D3DXMatrixMultiply(&matrix, (D3DXMATRIX*)matWorldView, (D3DXMATRIX*)matProj);

        D3DXMatrixTranspose(&matrix, &matrix);

        position.x = input.x * matrix._11 + input.y * matrix._12 + input.z * matrix._13 + input.w * matrix._14;
        position.y = input.x * matrix._21 + input.y * matrix._22 + input.z * matrix._23 + input.w * matrix._24;
        position.z = input.x * matrix._31 + input.y * matrix._32 + input.z * matrix._33 + input.w * matrix._34;
        position.w = input.x * matrix._41 + input.y * matrix._42 + input.z * matrix._43 + input.w * matrix._44;

        float xx, yy;
        xx = ((position.x / position.w) * (ViewportWidth / 2.0f)) + (ViewportWidth / 2.0f);
        yy = (ViewportHeight / 2.0f) + ((position.y / position.w) * (ViewportHeight / 2.0f));

        AimEspInfo_t pAimEspInfo = { static_cast<float>(xx), static_cast<float>(yy), static_cast<float>(position.w) };
        AimEspInfo.push_back(pAimEspInfo);
        
        OUTPUT_DEBUG(L"draw4 %d,%d,%d", xx, yy, mw);*/

        draw1_x = 200;
        draw1_y = 300;
    }


}


void __stdcall hkDrawIndexed11(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    bool matched = false;

    if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x31) & 1))
    {
        radio_stride++;
    }

    //OUTPUT_DEBUG(L"hkDrawIndexed11 >> IndexCount %d ,StartIndexLocation %d ,BaseVertexLocation %d\n", IndexCount, StartIndexLocation, BaseVertexLocation);
    // TODO mov key operation log in hkDrawIndexedInstancedIndirect?
    // change the stride 
    if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x35) & 1))
    {
        find_model_type = 2;
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
        }
    }
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x30) & 1))
    {
        if (current_count>0)
        {
            current_count = current_count - 1;
            current_item = table_items[current_count];
            OUTPUT_DEBUG(L"current_count %d/%d --> ( %d,%d,%d,%d,%d)", table_items.size(), current_count + 1, current_item.Stride, current_item.IndexCount, current_item.inWidth, current_item.veWidth, current_item.pscWidth);
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


    //OUTPUT_DEBUG(L"find_model_type %d, current_count %d,draw_type %d", find_model_type, current_count, draw_type)
    if (find_model_type ==1 && current_count >= 0)
    {
        if (current_item.Stride == Stride && current_item.IndexCount == IndexCount && current_item.veWidth == veWidth && current_item.pscWidth == pscWidth && current_item.inWidth == inWidth)
        {
            if ((GetAsyncKeyState(VK_END) & 1))
            {
                LOG_INFO("Table:Target obj is=\t(Stride==%d && IndexCount==%d && veWidth==%d && pscWidth==%d)", Stride, IndexCount, veWidth, pscWidth);
            }
            matched = true;
        }
    }
    if ((radio_stride == Stride && find_model_type == 2))
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
            OUTPUT_DEBUG(L"2. filter radio_inidex >> ( [%d,>> %d],%d,%d)",
                         Stride, IndexCount, veWidth, pscWidth
            );

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
                    if ((GetAsyncKeyState(VK_END) & 1))
                    {
                        LOG_INFO("Redio:Target obj is=\t(Stride==%d && IndexCount==%d && veWidth==%d && pscWidth==%d)", 
                                 Stride, IndexCount, veWidth, pscWidth,
                                 Stride, IndexCount, veWidth, pscWidth
                        );
                    }
                    OUTPUT_DEBUG(L"4. filter radio_psc_width >> ([%d,%d,%d,>> %d])", Stride, IndexCount, veWidth, pscWidth);
                }
            }
        }
    }
    if (matched && draw_type > 0)
    {
        if (draw_type == 1 || draw_type == 2) {

            pContext->OMGetDepthStencilState(&oDepthStencilState, &stencilRef); //need stencilref for optimisation later
            pContext->OMSetDepthStencilState(depthStencilStateFalse, stencilRef); //depth off
            if (draw_type == 2)
            {
                pContext->PSSetShader(sMagenta, NULL, NULL);
            }
            oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw
            pContext->OMSetDepthStencilState(oDepthStencilState, stencilRef); //depth on
            if (draw_type == 2)
            {
                pContext->PSSetShader(sGreen, NULL, NULL); // If you want all green,plz fuck this line;
            }
            SAFE_RELEASE(oDepthStencilState);
            
            // return after draw ? Prevent secondary rendering ?
            oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw
            //return oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw
            return;
        }
        else if (draw_type == 3)
        {
            pContext->PSSetShader(sGreen, NULL, NULL);
            oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw
            //return oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
            // return after draw ? Prevent secondary rendering ?
            return;
        }
        else if (draw_type == 4 || draw_type == 5)
        {
            if (draw_type == 4)
            {
                pContext->OMGetDepthStencilState(&oDepthStencilState, &stencilRef); //need stencilref for optimisation later
            }

            for (int x1 = 0; x1 <= 10; x1++)
            {
                pContext->PSSetShaderResources(x1, 1, &textureViewRed);
            }
            pContext->PSSetSamplers(0, 1, &pSamplerState);
            if (draw_type == 4)
            {
                pContext->OMSetDepthStencilState(depthStencilStateFalse, 0);
            }
            oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw
            if (draw_type == 4)
            {
                /*for (int x1 = 0; x1 <= 10; x1++)
                {
                    pContext->PSSetShaderResources(x1, 1, &textureViewGreen);
                }
                pContext->PSSetSamplers(0, 1, &pSamplerState);*/

                pContext->OMSetDepthStencilState(oDepthStencilState, stencilRef);
                SAFE_RELEASE(oDepthStencilState);
                oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation); //redraw
            }
            return; 
        }
        else {
            
            //AddModel(pContext); //w2s
            return;//delete selected texture
        }

    }
    return oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

uint32_t ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (r | (g << 8) | (b << 16) | (a << 24));
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

            //create depthstencilstate
            // DEPTH FALSE
            D3D11_DEPTH_STENCIL_DESC depthStencilDescFalse = {};
            //
            // Depth state:
            depthStencilDescFalse.DepthEnable = false;
            depthStencilDescFalse.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            
            //UT4 only
            depthStencilDescFalse.DepthFunc = D3D11_COMPARISON_GREATER; // D3D11_COMPARISON_ALWAYS
            //
            // Stencil state:
            depthStencilDescFalse.StencilEnable = true;
            depthStencilDescFalse.StencilReadMask = 0xFF;
            depthStencilDescFalse.StencilWriteMask = 0xFF;
            //
            // Stencil operations if pixel is front-facing:
            depthStencilDescFalse.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            depthStencilDescFalse.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
            depthStencilDescFalse.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            depthStencilDescFalse.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            //
            // Stencil operations if pixel is back-facing:
            depthStencilDescFalse.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            depthStencilDescFalse.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
            depthStencilDescFalse.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            depthStencilDescFalse.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            pDevice->CreateDepthStencilState(&depthStencilDescFalse, &depthStencilStateFalse);


            D3D11_TEXTURE2D_DESC d_desc;
            DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            memset(&d_desc, 0, sizeof(d_desc));
            d_desc.Width = 1;
            d_desc.Height = 1;
            d_desc.MipLevels = 1;
            d_desc.ArraySize = 1;
            d_desc.Format = format;
            d_desc.SampleDesc.Count = 1;
            d_desc.Usage = D3D11_USAGE_DEFAULT;
            d_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            HRESULT hr;
            static const uint32_t color_uint_red = 0xff0000ff;
            D3D11_SUBRESOURCE_DATA initDataRed = { &color_uint_red, sizeof(uint32_t), 0 };
            hr = pDevice->CreateTexture2D(&d_desc, &initDataRed, &textureRed);

            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            memset(&SRVDesc, 0, sizeof(SRVDesc));
            SRVDesc.Format = format;
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = 1;
            pDevice->CreateShaderResourceView(textureRed, &SRVDesc, &textureViewRed);



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

            ImGuiIO& io = ImGui::GetIO(); (void)io;
            ViewportWidth = io.DisplaySize.x;
            ViewportHeight = io.DisplaySize.y;
            ScreenCenterX = ViewportWidth / 2.0f;
            ScreenCenterY = ViewportHeight / 2.0f;

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
        static float bg_alpha = 0.5;
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        // greetings
        if (greetings)
        {
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


        //ImGuiIO& io = ImGui::GetIO();
        POINT mPos;
        GetCursorPos(&mPos);
        ScreenToClient(global::GAME_HWND, &mPos);
        ImGui::GetIO().MousePos.x = mPos.x;
        ImGui::GetIO().MousePos.y = mPos.y;
        if (GetAsyncKeyState(VK_INSERT) & 1) p_open = !p_open;
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
            ImGui::SliderInt("FovSize", &fov_size, 0, 200, "fov_size:%d");
            ImGui::SliderFloat("BGAlpha", &bg_alpha, 0.0f, 1.0f, "bg_alpha:%.1f");
            ImGui::NewLine();


            if (ImGui::CollapsingHeader("FindModel", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("FindModelType: ");
                ImGui::RadioButton("FindByTable", &find_model_type, 1); ImGui::SameLine();
                ImGui::RadioButton("FindBySlider", &find_model_type, 2);
                const char* items[] = { "None", "DrawZ", "DrawZ&DrawShaderColor", "DrawShaderColor","DrawZ&DrawTextureColor","DrawTextureColor", "DrawHide"};
                ImGui::Combo("DrawType: ", &draw_type, items, IM_ARRAYSIZE(items));

                /*ImGui::Text("DrawType: ");
                ImGui::RadioButton("None", &draw_type, 0); ImGui::SameLine();
                ImGui::RadioButton("DrawZ", &draw_type, 1); ImGui::SameLine();
                ImGui::RadioButton("DrawZ&DrawColor", &draw_type, 2); ImGui::SameLine();
                ImGui::RadioButton("DrawColor", &draw_type, 3); ImGui::SameLine();
                ImGui::RadioButton("DrawHide", &draw_type, 4);*/

                if (find_model_type == 1)
                {
                    if (ImGui::CollapsingHeader("FindByTable", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::Checkbox("RefreshDrawData", &refresh_draw_items); // TODO Auto stop ??

                        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
                        
                        if (current_count<=-1)
                        {
                            ImGui::Text("-> Plz type Alt+0/Ctrl+0 select: "); 
                        }
                        else {
                            ImGui::Text("-> %d: ( %d ,%d ,%d ,%d )", current_count + 1, current_item.Stride, current_item.IndexCount, current_item.veWidth, current_item.pscWidth); ImGui::SameLine();
                            ImGui::SameLine();
                            if (ImGui::Button("Copy"))
                            {
                                char result_str[100];
                                std::string result_s;
                                sprintf(result_str, "%d: %d,%d,%d,%d", current_count + 1, current_item.Stride, current_item.IndexCount, current_item.veWidth, current_item.pscWidth);
                                result_s = result_str;

                                if (::OpenClipboard(NULL))
                                {
                                    ::EmptyClipboard();
                                    HGLOBAL clipbuffer;
                                    char* buffer;
                                    clipbuffer = ::GlobalAlloc(GMEM_DDESHARE, strlen(result_s.c_str()) + 1);
                                    buffer = (char*)::GlobalLock(clipbuffer);
                                    strcpy(buffer, result_s.c_str());
                                    ::GlobalUnlock(clipbuffer);
                                    ::SetClipboardData(CF_TEXT, clipbuffer);
                                    ::CloseClipboard();
                                }
                            }
                        }
                        if (ImGui::BeginTable("BableAdvanced", 8, flags, ImVec2(0, 0), 0.0f))
                        {
                            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, DrawItemColumnID_ID);
                            ImGui::TableSetupColumn("Stride", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, DrawItemColumnID_Stride);
                            ImGui::TableSetupColumn("IndexCount", ImGuiTableColumnFlags_WidthFixed, 0.0f, DrawItemColumnID_IndexCount);
                            ImGui::TableSetupColumn("inWidth", ImGuiTableColumnFlags_DefaultHide, 0.0f, DrawItemColumnID_inWidth); // TODB TBD
                            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_WidthFixed, 0.0f, DrawItemColumnID_Action);// TODB TBD
                            ImGui::TableSetupColumn("veWidth", ImGuiTableColumnFlags_NoSort, 0.0f, DrawItemColumnID_veWidth);
                            ImGui::TableSetupColumn("pscWidth", ImGuiTableColumnFlags_NoSort, 0.0f, DrawItemColumnID_pscWidth);
                            ImGui::TableSetupColumn("Hidden", ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoSort);// TODB TBD
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
                    }
                } else if (find_model_type == 2)
                {
                    step_type = 3;
                    radio_stride = 40;
                    radio_inidex = 7;

                    if (ImGui::CollapsingHeader("FindBySlider", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::SliderInt("StepMode", &step_type, 1, 3, "Step Mode:%d");
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

                        ImGui::SliderInt("Stride", &radio_stride, -1, 100, "Stride:%d"); ImGui::SameLine(); common_imgui::HelpMarker("Alt + 1 (add)| Ctrl + 1 (min)");
                        ImGui::SliderInt("IndexCount", &radio_inidex, -1, 100, "IndexCount:%d"); ImGui::SameLine(); common_imgui::HelpMarker("Alt + 2 (add)| Ctrl + 2 (min)");
                        ImGui::SliderInt("veWidth", &radio_width, -1, 100, "veWidth:%d"); ImGui::SameLine(); common_imgui::HelpMarker("Alt + 3 (add)| Ctrl + 3 (min)");
                        ImGui::SliderInt("pscWidth", &radio_psc_width, -1, 100, "pscWidth:%d"); ImGui::SameLine(); common_imgui::HelpMarker("Alt + 4 (add)| Ctrl + 4 (min)");
                    }
                }
            }

            ImGui::PushID(1);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(7.0f, 0.8f, 0.8f));
            ImGui::NewLine();
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
                        sGreen->Release();
                        sMagenta->Release();
                        SAFE_RELEASE(depthStencilStateFalse);
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
                // TODO return void??
                return oPresent(pSwapChain, SyncInterval, Flags);
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            ImGui::End();
        }

        const auto draw = ImGui::GetBackgroundDrawList();
        static const auto size = ImGui::GetIO().DisplaySize;
        static const auto center = ImVec2(size.x / 2, size.y / 2);
        if (draw_fov)
            draw->AddCircle(center, fov_size, ImColor(255, 255, 255), 100);

        if (draw_filled_fov)
            draw->AddCircleFilled(center, fov_size, ImColor(0, 0, 0, 140), 100);

        if (draw1_x != -1 && draw1_y != -1)
        {
            draw->AddText(ImVec2(draw1_x, draw1_y), color_red, "this is draw1");
        }
        draw->AddText(ImVec2(100, 100), color_red, "this is demo");

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
        LOG_INFO("GetModuleHandle with dxgi.dll..");
    } while (!hDXGIDLL);
    Sleep(100);


    IDXGISwapChain* pSwapChain;

    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
    RegisterClassExA(&wc);
    HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
    LOG_INFO("CreateWindowA >> hWnd {%d}",hWnd);
    
    D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    D3D_FEATURE_LEVEL obtainedLevel;

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

    LOG_INFO("Create directX device and swapchain!");
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

    LOG_INFO("DetourTransactionBegin will be hooked >> oPresent {%d}, oDrawIndexed{%d}",oPresent,oDrawIndexed);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(LPVOID&)oPresent, (PBYTE)hkPresent11);
    DetourAttach(&(LPVOID&)oDrawIndexed, (PBYTE)hkDrawIndexed11);
    DetourTransactionCommit();
    LOG_INFO("DetourTransactionBegin hook complete >> hkPresent11 {%d}, hkDrawIndexed11{%d}",hkPresent11,hkDrawIndexed11);

    DWORD dwOld;
    VirtualProtect(oPresent, 2, PAGE_EXECUTE_READWRITE, &dwOld);
    
    LOG_INFO("VirtualProtect >> oPresent {%d}, dwOld{ %d}",oPresent,dwOld);

    /*while (true) {
        Sleep(10);
    }*/

    Sleep(500);

    pDevice->Release();
    pContext->Release();
    pSwapChain->Release();

    LOG_INFO("Device Release >> pDevice, pContext, pSwapChain");

}


