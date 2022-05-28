#pragma once
#include "imgui/imgui.h"
enum
{
    FL_NONE = 1 << 0,
    FL_SHADOW = 1 << 1,
    FL_OUTLINE = 1 << 2,
    FL_CENTER_X = 1 << 3,
    FL_CENTER_Y = 1 << 4
};
bool WorldToScreen(float position[3], float screen[2], float matrix[16], int windowWidth, int windowHeight);
void DrawTextVal(int x, int y, const ImColor& color, const char* val);
void AddImage(const ImVec2& position, const ImVec2& size, const ImTextureID pTexture, const ImColor& color);
void AddCircleFilled(const ImVec2& position, float radius, const ImColor& color, int segments = 100);
void AddCircle(const ImVec2& position, float radius, const ImColor& color, int segments = 100);
void AddRectFilled(const ImVec2& position, const ImVec2& size, const ImColor& color, float rounding = 0.f);
void AddRectFilledGradient(const ImVec2& position, const ImVec2& size, const ImColor& leftTop, const ImColor& rightTop, const ImColor& leftBot, const ImColor& rightBot);
void DrawFillArea(float x, float y, float w, float h, const ImColor& color, float rounding = 0.f);
void DrawFillAreaGradient(float x, float y, float w, float h, const ImColor& leftTop, const ImColor& rightTop, const ImColor& leftBot, const ImColor& rightBot);
void AddTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImColor& color);
void AddTriangleFilled(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImColor& color);
void AddLine(const ImVec2& from, const ImVec2& to, const ImColor& color, float thickness = 1.f);
void DrawLines(int x0, int y0, int x1, int y1, int r, int g, int b, int a);
void AddText(float x, float y, const ImColor& color, float fontSize, int flags, const char* format, ...);
void AddRect(const ImVec2& position, const ImVec2& size, const ImColor& color, float rounding = 0.f);
void DrawBox(float x, float y, float w, float h, const ImColor& color);
void DrawBoxOutline(float x, float y, float w, float h, const ImColor& color);
void DrawRoundBox(float x, float y, float w, float h, const ImColor& color, float rounding);
void DrawRoundBoxOutline(float x, float y, float w, float h, const ImColor& color, float rounding);
void DrawCornerBox(float x, float y, float w, float h, const ImColor& color);
void DrawCornerBoxOutline(float x, float y, float w, float h, const ImColor& color);
void DrawEspBox(int box_type, float x, float y, float w, float h, float r, float g, float b, float a);
void DrawDot(int x, int y, const ImColor& color);
void Draw3DBox(int x, int y,int w,int h,int offset, const ImColor& color);

