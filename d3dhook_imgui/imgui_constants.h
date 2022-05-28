#pragma once
#include <wtypes.h>
#include "imgui/imgui.h"
//#include "plugins.h"
#pragma comment(lib,"d3d11.lib")
#include "resource.h"


enum DrawItemColumnID
{
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

#if defined(_M_X64)	
# define WNDPROC_INDEX GWLP_WNDPROC
# define IS_X64 true
#else
# define WNDPROC_INDEX GWL_WNDPROC
# define IS_X64 false
#endif


#define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL; }
#define DEPTH_BIAS_D32_FLOAT(d) (d/(1/pow(2,23)))


extern HMODULE Dll_HWND;
extern HWND GAME_HWND;
extern float HWND_SCREEN_X;
extern float HWND_SCREEN_Y;

extern struct ImColor color_red;
extern struct ImColor color_green;
extern struct ImColor color_blue;
extern struct ImVec4 color_pick;


extern int p_open;
extern bool greetings;
extern bool init;
extern bool refresh_draw_items;
extern bool draw_demo;
extern int draw1_x;
extern int draw1_y;
extern int id_number;

extern bool draw_fov;
extern bool draw_filled_fov;
extern bool draw_double_color;
extern int fov_size;
extern float bg_alpha;
extern float rounding;
extern int segments;
extern int box_type;

extern int radio_stride;
extern int radio_inidex;
extern int radio_width;
extern int radio_psc_width;
extern int step_type;
extern int has_focus;



extern int find_model_type;
extern struct DrawItem current_item;
extern int current_count;

extern int wall_hack_type;
extern const char* wall_hack_type_items[];
extern int wall_hack_type_items_len;

extern int draw_cclor_type;
extern const char* draw_cclor_type_items[];
extern int draw_cclor_type_items_len;

extern const char* find_modul_items[];
extern int find_modul_items_len;

extern const char* has_focus_items[];


extern void DrawGreetWin();
extern void DrawMainWin();

extern void doGl3wInit();

extern ImVector<DrawItem> table_items;
extern ImVector<int> selection;

extern WNDPROC oWndProcHandler;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct EPINFO
{
    DWORD pid;
    HWND hwnd;
};

extern HWND GetHwndByPid(DWORD dwProcessID);
extern HWND GetMainHWnd(DWORD pid);