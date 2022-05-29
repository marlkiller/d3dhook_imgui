#include <wtypes.h>
#include "imgui/imgui.h"
#include "imgui_constants.h"
#include <string>
#include "imgui_draw_util.h"
#include "common_utils.h"
#include "imgui/imgui_internal.h"
#include "imgui/gl3w.h"
#include "logger.h"


HMODULE Dll_HWND = nullptr;
HWND GAME_HWND = nullptr;
float HWND_SCREEN_X;
float HWND_SCREEN_Y;

ImColor color_red = ImColor(255, 0, 0, 255);
ImColor color_green = ImColor(0, 255, 0, 255);
ImColor color_blue = ImColor(0, 0, 255, 255);
ImColor color_black = ImColor(255, 255, 255, 255);
ImVec4 color_pick = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

int p_open = 0;
bool greetings = true;
bool init = false;
bool refresh_draw_items = false;
bool draw_demo = false;
int draw1_x = -1;
int draw1_y = -1;
int id_number = 0;

bool draw_fov = false;
bool draw_filled_fov = false;
int fov_size = 100;
float bg_alpha = 0.5;
float rounding = 6;
int segments = 100;
int box_type = 3;

bool draw_double_color = false;


int radio_stride = -1;
int radio_inidex = -1;
int radio_width = -1;
int radio_psc_width = -1;
int step_type = 1;
int has_focus = 0;



int find_model_type = 0;
DrawItem current_item;
int current_count = -1;

int wall_hack_type = 0;
const char* wall_hack_type_items[] = { "None", "DrawZ", "DrawDept", "DrawHide"};
int wall_hack_type_items_len = sizeof(wall_hack_type_items) / sizeof(wall_hack_type_items[0]);

int draw_cclor_type = 0;
const char* draw_cclor_type_items[] = { "None", "DrawShaderColor", "DrawTextureColor"};
int draw_cclor_type_items_len = sizeof(draw_cclor_type_items) / sizeof(draw_cclor_type_items[0]);;

const char* find_modul_items[] = { "None","FindByTable", "FindBySlider" };
int find_modul_items_len = sizeof(find_modul_items) / sizeof(find_modul_items[0]);

const char* has_focus_items[] = { "None","FindModelType", "WallHack" ,"DrawColor" ,"StepModel"};
 
ImVector<DrawItem> table_items;
ImVector<int> selection;

WNDPROC oWndProcHandler = nullptr;

void doGl3wInit(){
    gl3wInit();
}

void PushColor() {
    ImGui::PushID(3);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(3 / 7.0f, 0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(3 / 7.0f, 0.6f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(3 / 7.0f, 0.7f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(3 / 7.0f, 0.9f, 0.9f));
}


void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}


void DrawMainWin()
{
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (GetAsyncKeyState(VK_INSERT) & 1) p_open = !p_open;
    io.MouseDrawCursor = p_open;
   /* io.WantCaptureMouse = p_open;
    io.WantCaptureKeyboard = p_open;*/
    //LOG_INFO("p_open %d", p_open);
    ImGuiContext& g = *GImGui;
    HWND_SCREEN_X = g.Viewports[0]->Size.x;
    HWND_SCREEN_Y = g.Viewports[0]->Size.y;

    if (p_open)
    {
        ImGui::SetNextWindowBgAlpha(bg_alpha);
        ImGui::Begin("My Windows ");
        ImGui::Text("Application average \n%.3f ms/frame (%.1f FPS) , Mouse pos: (%g, %g), Wnd :(%f,%f)", 1000.0f / io.Framerate, io.Framerate, io.MousePos.x, io.MousePos.y, HWND_SCREEN_X, HWND_SCREEN_Y);
        
        ImGui::Text("Item with focus: %s", has_focus_items[has_focus]);
        
        ImGui::Checkbox("DrawDemo", &draw_demo);
        //ImGui::ColorPicker4("##picker", (float*)&color_pick , ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_AlphaPreview);
        if (draw_demo)
        {
            ImGui::ColorEdit4("HSV RGB ColorPick", (float*)&color_pick, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float| ImGuiColorEditFlags_AlphaBar);
            ImGui::SliderFloat("BGAlpha", &bg_alpha, 0.0f, 1.0f, "bg_alpha:%.1f");
            ImGui::SliderFloat("Rounding", &rounding, 0.0f, 12.0f, "rounding:%.1f");
            ImGui::SliderInt("Segments", &segments, 0, 512, "segments:%d");
            ImGui::SliderInt("BoxType", &box_type, 1, 6, "box_type:%d");
            ImGui::NewLine();
        }
     


        if (ImGui::CollapsingHeader("FindModel", ImGuiTreeNodeFlags_DefaultOpen))
        {


            if ((GetAsyncKeyState(VK_UP) & 1) && has_focus > 0) {
                has_focus--;
            }
            else if ((GetAsyncKeyState(VK_DOWN) & 1) && has_focus < 4) {
                has_focus++;
            }
            if (has_focus > 0)
            {
                if (GetAsyncKeyState(VK_LEFT) & 1) {
                    switch (has_focus)
                    {
                    case 1:
                        if (find_model_type > 0)
                        {
                            find_model_type--;
                        }
                        break;
                    case 2:
                        if (wall_hack_type > 0)
                        {
                            wall_hack_type--;
                        }
                        break;
                    case 3:
                        if (draw_cclor_type > 0)
                        {
                            draw_cclor_type--;
                        }
                        break;
                    case 4:
                        if (step_type > 0)
                        {
                            step_type--;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else if (GetAsyncKeyState(VK_RIGHT) & 1) {
                    switch (has_focus)
                    {
                    case 1:
                        if (find_model_type < find_modul_items_len - 1)
                        {
                            find_model_type++;
                        }
                        break;
                    case 2:
                        if (wall_hack_type < wall_hack_type_items_len - 1)
                        {
                            wall_hack_type++;
                        }
                        break;
                    case 3:
                        if (draw_cclor_type < draw_cclor_type_items_len - 1)
                        {
                            draw_cclor_type++;
                        }
                        break;
                    case 4:
                        if (step_type < 3)
                        {
                            step_type++;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }


            if (has_focus == 1) { PushColor(); };
            ImGui::SliderInt("FindModelType: ", &find_model_type, 0, find_modul_items_len - 1, find_modul_items[find_model_type], ImGuiSliderFlags_NoInput);
            if (has_focus == 1) { ImGui::PopStyleColor(4); ImGui::PopID(); };

            if (has_focus == 2) { PushColor(); };
            ImGui::SliderInt("WallHack: ", &wall_hack_type, 0, wall_hack_type_items_len - 1, wall_hack_type_items[wall_hack_type], ImGuiSliderFlags_NoInput);
            if (has_focus == 2) { ImGui::PopStyleColor(4); ImGui::PopID(); };

            if (has_focus == 3) { PushColor(); };
            ImGui::SliderInt("DrawColor: ", &draw_cclor_type, 0, draw_cclor_type_items_len - 1, draw_cclor_type_items[draw_cclor_type], ImGuiSliderFlags_NoInput); ImGui::SameLine();
            ImGui::Checkbox("DrawDoubleColor", &draw_double_color);
            if (has_focus == 3) { ImGui::PopStyleColor(4); ImGui::PopID(); };

            if (find_model_type == 1)
            {
                if (ImGui::CollapsingHeader("FindByTable", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("RefreshDrawData", &refresh_draw_items); // TODO Auto stop ??


                    static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

                    if (current_count <= -1)
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
            }
            else if (find_model_type == 2)
            {
                if (ImGui::CollapsingHeader("FindBySlider", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (has_focus == 4) { PushColor(); };
                    ImGui::SliderInt("StepMode", &step_type, 1, 3, "Step Mode:%d");
                    if (has_focus == 4) { ImGui::PopStyleColor(4); ImGui::PopID(); };

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
                    ImGui::SliderInt("Stride", &radio_stride, -1, 100, "Stride:%d"); /*if (ImGui::IsItemActive()) { has_focus = 4; }; */ImGui::SameLine(); HelpMarker("Alt + 1 (add)| Ctrl + 1 (min)");
                    ImGui::SliderInt("IndexCount", &radio_inidex, -1, 100, "IndexCount:%d");/* if (ImGui::IsItemActive()) { has_focus = 5; };*/ ImGui::SameLine(); HelpMarker("Alt + 2 (add)| Ctrl + 2 (min)");
                    ImGui::SliderInt("veWidth", &radio_width, -1, 100, "veWidth:%d"); /*if (ImGui::IsItemActive()) { has_focus = 6; };*/ ImGui::SameLine(); HelpMarker("Alt + 3 (add)| Ctrl + 3 (min)");
                    ImGui::SliderInt("pscWidth", &radio_psc_width, -1, 100, "pscWidth:%d"); /*if (ImGui::IsItemActive()) { has_focus = 7; };*/ ImGui::SameLine(); HelpMarker("Alt + 4 (add)| Ctrl + 4 (min)");


                }
            }


        }
        /*ImGui::PushID(1);
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

            SetWindowLongPtr(GAME_HWND, WNDPROC_INDEX, (LONG_PTR)OriginalWndProcHandler);
            LOG_INFO("Detach with {%d},{%d},{%d}", GAME_HWND, WNDPROC_INDEX, OriginalWndProcHandler);
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
                    FreeLibrary(Dll_HWND);

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
        ImGui::PopID();*/

        ImGui::End();
    }

   

    if (draw_demo)
    {


        ImColor current_color = ImColor(color_pick);

        const auto draw = ImGui::GetBackgroundDrawList();
        static const auto size = ImGui::GetIO().DisplaySize;
        static const auto center = ImVec2(size.x / 2, size.y / 2);

        draw->AddCircle(center, fov_size, current_color, 100);
        draw->AddCircleFilled(center, fov_size, ImColor(0, 0, 0, 140), 100);
        draw->AddText(ImVec2(100, 100), current_color, "this is text demo");


        int start_x = 150;
        int start_y = 150;
        int w = 100;
        int h = 200;

        AddCircle(ImVec2(start_x, start_y), fov_size, current_color, segments); start_x += 150; start_y -= fov_size;
        AddRectFilled(ImVec2(start_x, start_y),ImVec2(start_x+100, start_y+200), current_color, rounding); start_x += 150;
        AddRectFilledGradient(ImVec2(start_x, start_y), ImVec2(start_x + 100, start_y + 200), color_black, color_red, color_green, color_blue); start_x += 150;
        DrawBox(start_x, start_y, w,h, current_color); start_x += 150;
        DrawBoxOutline(start_x, start_y, w, h, current_color); start_x += 150;
        DrawRoundBox(start_x, start_y, w, h, current_color, rounding);

        start_x = 100; start_y = 300;
        DrawCornerBox(start_x, start_y, w, h, current_color); start_x += 150;
        DrawEspBox(box_type, start_x, start_y, w, h, 255,255,255,255); start_x += 150;
        DrawDot(start_x, start_y, current_color); start_x += 150;
            

        // 3D Box
        int x1 = 650; int y1 = 350;
        int offset = 50;

        Draw3DBox(x1, y1, w, h, offset, current_color);
    }
}

void DrawGreetWin() {
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
            //LOG_INFO("greetings init timePassed {%d}", timePassed);
            greetings = false;
            lastTime = timeGetTime();
        }
    }
}

LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    if (p_open)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        return true;
    }

return CallWindowProc(oWndProcHandler, hWnd, uMsg, wParam, lParam);
}

static BOOL CALLBACK cbEnumProc(HWND hwnd, LPARAM lparam)
{
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    EPINFO* pwi = (EPINFO*)lparam;
    if (pid == pwi->pid)
    {
        HWND _hwnd = GetParent(hwnd);
        if (_hwnd != NULL)
        {
            pwi->hwnd = _hwnd;
            return FALSE;
        }
    }

    return TRUE;
}

HWND GetHwndByPid(DWORD dwProcessID)
{
    //返回Z序顶部的窗口句柄
    HWND hWnd = ::GetTopWindow(0);

    while (hWnd)
    {
        DWORD pid = 0;
        //根据窗口句柄获取进程ID
        DWORD dwTheardId = ::GetWindowThreadProcessId(hWnd, &pid);
       
        if (dwTheardId != 0)
        {
            if (pid == dwProcessID)
            {
                char buf[128];
                GetWindowText(hWnd, buf, sizeof(buf));
                bool filter = true;

                if (strstr(buf, "MSCTFIME UI")|| strstr(buf, "Default IME")|| strstr(buf, "[Console-Wind]")) {
                    filter = false;
                    LOG_INFO("Win title is igonre { %s } - {%d}", buf, hWnd);
                }
               
                if (filter) {
                    LOG_INFO("Target Win title is { %s } - {%d}", buf, hWnd);
                    return hWnd;
                }
                    
            }
        }
        //返回z序中的前一个或后一个窗口的句柄
        hWnd = ::GetNextWindow(hWnd, GW_HWNDNEXT);

    }
    return hWnd;
}

// 获取主窗口句柄
HWND GetMainHWnd(DWORD pid)
{
    EPINFO wi{ 0 };
    wi.pid = pid;
    EnumWindows(cbEnumProc, (LPARAM)&wi);

    return wi.hwnd;
}