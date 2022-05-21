
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui_draw_util.h"

void AddImage(const ImVec2& position, const ImVec2& size, const ImTextureID pTexture, const ImColor& color)
{
    ImRect bb(position, ImVec2(position.x + size.x, position.y + size.y));

    ImGui::GetBackgroundDrawList()->AddImage(pTexture, bb.Min, bb.Max, ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImGui::ColorConvertFloat4ToU32(color));
}

void AddCircleFilled(const ImVec2& position, float radius, const ImColor& color, int segments)
{
    ImGui::GetBackgroundDrawList()->AddCircleFilled(position, radius, ImGui::ColorConvertFloat4ToU32(color), segments);
}

void AddCircle(const ImVec2& position, float radius, const ImColor& color, int segments)
{
    ImGui::GetBackgroundDrawList()->AddCircle(position, radius, ImGui::ColorConvertFloat4ToU32(color), segments);
}

void AddRectFilled(const ImVec2& position, const ImVec2& size, const ImColor& color, float rounding)
{
    ImGui::GetBackgroundDrawList()->AddRectFilled(position, size, ImGui::ColorConvertFloat4ToU32(color), rounding);
}

void AddRectFilledGradient(const ImVec2& position, const ImVec2& size, const ImColor& leftTop, const ImColor& rightTop, const ImColor& leftBot, const ImColor& rightBot)
{
    ImGui::GetBackgroundDrawList()->AddRectFilledMultiColor(position, size, (leftTop), (rightTop), (rightBot), (leftBot));
}

void AddRect(const ImVec2& position, const ImVec2& size, const ImColor& color, float rounding)
{
    ImGui::GetBackgroundDrawList()->AddRect(position, size, ImGui::ColorConvertFloat4ToU32(color), rounding);
}

void DrawFillArea(float x, float y, float w, float h, const ImColor& color, float rounding)
{
    AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color, rounding);
}

void DrawFillAreaGradient(float x, float y, float w, float h, const ImColor& leftTop, const ImColor& rightTop, const ImColor& leftBot, const ImColor& rightBot)
{
    AddRectFilledGradient(ImVec2(x, y), ImVec2(x + w, y + h), (leftTop), (rightTop), (rightBot), (leftBot));
}

void AddTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImColor& color)
{
    ImGui::GetBackgroundDrawList()->AddTriangle(a, b, c, ImGui::ColorConvertFloat4ToU32(color));
}

void AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImColor& color)
{
    ImGui::GetBackgroundDrawList()->AddTriangleFilled(a, b, c, ImGui::ColorConvertFloat4ToU32(color));
}

void AddLine(const ImVec2& from, const ImVec2& to, const ImColor& color, float thickness)
{
    ImGui::GetBackgroundDrawList()->AddLine(from, to, ImGui::ColorConvertFloat4ToU32(color), thickness);
}

void DrawLines(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
    AddLine(ImVec2((float)x0, (float)y0), ImVec2((float)x1, (float)y1), ImColor(r, g, b));
}

void AddText(float x, float y, const ImColor& color, float fontSize, int flags, const char* format, ...)
{
        if (!format)
            return;
    
        auto& io = ImGui::GetIO();
        auto DrawList = ImGui::GetBackgroundDrawList();
        auto Font = io.FontDefault;
    
        char szBuff[256] = { '\0' };
        va_list vlist = nullptr;
        va_start(vlist, format);
        vsprintf_s(szBuff, format, vlist);
        va_end(vlist);
    
        DrawList->PushTextureID(io.Fonts->TexID);
    
        float size_font = fontSize;
    
        float size = size_font == 0.f ? Font->FontSize : size_font;
        ImVec2 text_size = Font->CalcTextSizeA(size, FLT_MAX, 0.f, szBuff);
    
        ImColor Color = ImColor(0.f, 0.f, 0.f, color.Value.w);
    
        if (flags & FL_CENTER_X)
            x -= text_size.x / 2.f;
    
        if (flags & FL_CENTER_Y)
            y -= text_size.x / 2.f;
    
    
        DrawList->AddText(Font, size, ImVec2(x + 1.f, y + 1.f), ImGui::ColorConvertFloat4ToU32(Color), szBuff);
    
        
        DrawList->AddText(Font, size, ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(color), szBuff);
        DrawList->PopTextureID();
}

void DrawBox(float x, float y, float w, float h, const ImColor& color)
{
    AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color);
}

void DrawBoxOutline(float x, float y, float w, float h, const ImColor& color)
{
    DrawBox(x + 1.f, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawBox(x - 1.f, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawBox(x - 1.f, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawBox(x + 1.f, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));

    DrawBox(x + 1.f, y, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawBox(x - 1.f, y, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawBox(x, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawBox(x, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));

    DrawBox(x, y, w, h, color);
}

void DrawRoundBox(float x, float y, float w, float h, const ImColor& color, float rounding)
{
    AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, rounding);
}

void DrawRoundBoxOutline(float x, float y, float w, float h, const ImColor& color, float rounding)
{
    DrawRoundBox(x + 1.f, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w), rounding);
    DrawRoundBox(x - 1.f, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w), rounding);
    DrawRoundBox(x - 1.f, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w), rounding);
    DrawRoundBox(x + 1.f, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w), rounding);

    DrawRoundBox(x + 1.f, y, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w), rounding);
    DrawRoundBox(x - 1.f, y, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w), rounding);
    DrawRoundBox(x, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w), rounding);
    DrawRoundBox(x, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w), rounding);

    DrawRoundBox(x, y, w, h, color, rounding);
}

void DrawCornerBox(float x, float y, float w, float h, const ImColor& color)
{
    AddLine(ImVec2(x, y), ImVec2(x + w / 4.f, y), color);
    AddLine(ImVec2(x, y), ImVec2(x, y + h / 4.f), color);

    AddLine(ImVec2(x + w, y), ImVec2(x + w - w / 4.f, y), color);
    AddLine(ImVec2(x + w, y), ImVec2(x + w, y + h / 4.f), color);

    AddLine(ImVec2(x, y + h), ImVec2(x + w / 4.f, y + h), color);
    AddLine(ImVec2(x, y + h), ImVec2(x, y + h - h / 4.f), color);

    AddLine(ImVec2(x + w, y + h), ImVec2(x + w, y + h - h / 4.f), color);
    AddLine(ImVec2(x + w, y + h), ImVec2(x + w - w / 4.f, y + h), color);
}

void DrawCornerBoxOutline(float x, float y, float w, float h, const ImColor& color)
{
    DrawCornerBox(x - 1.f, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawCornerBox(x - 1.f, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawCornerBox(x + 1.f, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawCornerBox(x + 1.f, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));

    DrawCornerBox(x - 1.f, y, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawCornerBox(x, y - 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawCornerBox(x, y + 1.f, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));
    DrawCornerBox(x + 1.f, y, w, h, ImColor(0.f, 0.f, 0.f, color.Value.w));

    DrawCornerBox(x, y, w, h, color);
}

void DrawEspBox(int box_type, float x, float y, float w, float h, float r, float g, float b, float a)
{
    switch (box_type)
    {
    case 1:
        DrawBox(x, y, w, h, ImColor(r, g, b, a));
        break;
    case 2:
        DrawBoxOutline(x, y, w, h, ImColor(r, g, b, a));
        break;
    case 3:
        DrawCornerBox(x, y, w, h, ImColor(r, g, b, a));
        break;
    case 4:
        DrawCornerBoxOutline(x, y, w, h, ImColor(r, g, b, a));
        break;
    case 5:
        DrawRoundBox(x, y, w, h, ImColor(r, g, b, a), 3.f);
        break;
    case 6:
        DrawRoundBoxOutline(x, y, w, h, ImColor(r, g, b, a), 3.f);
        break;
    }
}

void DrawDot(int x, int y, const ImColor& color)
{
    float thickness = 4.f;

    DrawFillArea(x - thickness, y - thickness, thickness + 1.f, thickness + 1.f, color);
}

void Draw3DBox(int x, int y, int w, int h, int o, const ImColor& color)


{
    /*AddLine(ImVec2((float)x, (float)y), ImVec2(float(x + w), float(y)), color);
    AddLine(ImVec2((float)x, (float)y), ImVec2(float(x), float(y + h)), color);
    AddLine(ImVec2((float)x, (float)(y + h)), ImVec2(float(x + w), float(y + h)), color);
    AddLine(ImVec2(x + w, y + h), ImVec2(float(x + w), float(y)), color);

    AddLine(ImVec2((float)x, (float)y), ImVec2(float(x + o), float(y - o)), color);
    AddLine(ImVec2((float)(x + o), (float)(y - o)), ImVec2(float(x + o + w), float(y - o)), color);
    AddLine(ImVec2((float)(x + o + w), (float)(y - o)), ImVec2(float(x + w), float(y)), color);

    AddLine(ImVec2((float)(x + o + w), (float)(y - o)), ImVec2(float(x + o + w), float(y - o + h)), color);
    AddLine(ImVec2((float)(x + o + w), (float)(y - o + h)), ImVec2(float(x + w), float(y + h)), color);

    AddLine(ImVec2((float)(x + o), (float)(y - o)), ImVec2(float(x + o), float(y - o + h)), color);
    AddLine(ImVec2((float)(x + o), (float)(y - o + h)), ImVec2(float(x), float(y + h)), color);
    AddLine(ImVec2((float)(x + o), (float)(y - o + h)), ImVec2(float(x + o + w), float(y - o + h)), color);*/

    AddLine(ImVec2(x, y), ImVec2((x + w), (y)), color);
    AddLine(ImVec2(x, y), ImVec2((x), (y + h)), color);
    AddLine(ImVec2(x, (y + h)), ImVec2((x + w), (y + h)), color);
    AddLine(ImVec2(x + w, y + h), ImVec2((x + w), (y)), color);

    AddLine(ImVec2(x, y), ImVec2((x + o), (y - o)), color);
    AddLine(ImVec2((x + o), (y - o)), ImVec2((x + o + w), (y - o)), color);
    AddLine(ImVec2((x + o + w), (y - o)), ImVec2((x + w), (y)), color);

    AddLine(ImVec2((x + o + w), (y - o)), ImVec2((x + o + w), (y - o + h)), color);
    AddLine(ImVec2((x + o + w), (y - o + h)), ImVec2((x + w), (y + h)), color);

    AddLine(ImVec2((x + o), (y - o)), ImVec2((x + o), (y - o + h)), color);
    AddLine(ImVec2((x + o), (y - o + h)), ImVec2((x), (y + h)), color);
    AddLine(ImVec2((x + o), (y - o + h)), ImVec2((x + o + w), (y - o + h)), color);

}
