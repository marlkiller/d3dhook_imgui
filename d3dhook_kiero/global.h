#pragma once
#include <wtypes.h>
#include "imgui/imgui.h"
#include "imgui_plugin.h"

#if defined(_M_X64)	
# define WNDPROC_INDEX GWLP_WNDPROC
# define IS_X64 true
#else
# define WNDPROC_INDEX GWL_WNDPROC
# define IS_X64 false
#endif


#define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL; }
#define DEPTH_BIAS_D32_FLOAT(d) (d/(1/pow(2,23)))

namespace global
{
	extern HMODULE Dll_HWND;
	extern HWND GAME_HWND;
	
}

static ImColor color_red = ImColor(255, 0, 0, 255);
static bool p_open = false;
static bool greetings = true;
static bool init = false;
static bool refresh_draw_items = false;
static bool draw_demo = false;
static int draw1_x = -1;
static int draw1_y = -1;
static int id_number = 0;
static int draw_type = 0;



static int radio_stride = -1;
static int radio_inidex = -1;
static int radio_width = -1;
static int radio_psc_width = -1;
static int step_type = 1;
static int has_focus = 0;

static int find_model_type = 0;
static DrawItem current_item;
static int current_count = -1;

static const char* draw_type_items[] = { "None", "DrawZ", "DrawZ&DrawShaderColor", "DrawShaderColor","DrawZ&DrawTextureColor","DrawTextureColor","DrawDept", "DrawHide","dev" };
static int draw_type_items_len = sizeof(draw_type_items) / sizeof(draw_type_items[0]);

static const char* find_modul_items[] = { "None","FindByTable", "FindBySlider" };
static int find_modul_items_len = sizeof(find_modul_items) / sizeof(find_modul_items[0]);

static const char* has_focus_items[] = { "None","FindModelType", "DrawType" ,"StepMode" };

