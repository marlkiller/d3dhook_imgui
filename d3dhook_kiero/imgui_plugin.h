#pragma once



#include "imgui\imconfig.h"
#include "imgui\imgui.h"
#include "imgui\imgui_impl_dx11.h"
#include "imgui\imgui_impl_win32.h"
#include "imgui\imgui_internal.h"
#include "imgui\imstb_rectpack.h"
#include "imgui\imstb_textedit.h"
#include "imgui\imstb_truetype.h"


#include <d3d11.h>
#pragma comment(lib,"d3d11.lib")

namespace common_imgui
{
    void HelpMarker(const char* desc);
}



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


struct AppLog
{
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.

    AppLog()
    {
        AutoScroll = true;
        Clear();
    }

    void    Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void    Draw(const char* title, bool* p_open = NULL)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = Buf.begin();
        const char* buf_end = Buf.end();
        if (Filter.IsActive())
        {
            for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
            {
                const char* line_start = buf + LineOffsets[line_no];
                const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                if (Filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            ImGuiListClipper clipper;
            clipper.Begin(LineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
};


struct Funcs
{
    static int MyCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
        {
            data->InsertChars(data->CursorPos, "..");
        }
        else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
        {
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, "Pressed Up!");
                data->SelectAll();
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, "Pressed Down!");
                data->SelectAll();
            }
        }
        else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit)
        {
            // Toggle casing of first character
           // Buf
            printf("Buf :%s\n", data->Buf);

        }
        return 0;
    }
};
