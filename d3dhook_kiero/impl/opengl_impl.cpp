#include "../imgui_constants.h"
#include "opengl_impl.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../logger.h"
#include "../detours.h"

#include <exception>
#include "../imgui/imgui_impl_opengl2.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../imgui/GL.h"


typedef BOOL(APIENTRY* wglSwapBuffers)(HDC  hdc);
typedef void (APIENTRY* glBegin_t)(GLenum mode);
//typedef void (APIENTRY* glColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
//typedef void (APIENTRY* glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
//typedef void (APIENTRY* glClear)(GLbitfield mask);
//typedef void (APIENTRY* glVertex3f)(GLfloat x, GLfloat y, GLfloat z);


wglSwapBuffers oWGLSwapBuffers;
glBegin_t oGLBegin;


BOOL __stdcall hkWGLSwapBuffers(HDC hdc)
{

	try
	{
		if (!init) 
		{
			auto tStatus = true;

			LOG_INFO("imgui first init {%d}", init);
			GAME_HWND = WindowFromDC(hdc);
			oWndProcHandler = (WNDPROC)SetWindowLongPtr(GAME_HWND, WNDPROC_INDEX, (LONG_PTR)hWndProc);

			/*gglGetIntegerv(GL_MAJOR_VERSION, &iMajor);
			lGetIntegerv(GL_MINOR_VERSION, &iMinor);*/

			/*if ((iMajor * 10 + iMinor) >= 32)
				bOldOpenGL = false;*/
			//LOG_INFO("Is bOldOpenGL -> {%d}", bOldOpenGL);

			// TODO Support OpenGL3
			bool bOldOpenGL = false;

			ImGui::CreateContext();
			ImGui_ImplWin32_Init(GAME_HWND);
		  

			ImGui_ImplOpenGL2_Init();
			init = true;
		}


		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		DrawMainWin();

		//DrawRect();

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());



	}
	catch (...) {
		std::exception_ptr p = std::current_exception();
		LOG_ERROR("ERROR");
		LOG_ERROR("ERROR : {%s}", p);
		throw;
	}

	return oWGLSwapBuffers(hdc);
}


void APIENTRY hkGLBegin(GLenum mode)
{
	//OUTPUT_DEBUG(L"hkGLBegin >> {%d}", mode);

	bool matched = false;
	// change the mode
	if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x31) & 1))
	{
		radio_stride++;
	}
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x31) & 1))
	{
		radio_stride--;
	}
	// change the current_item
	if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(0x30) & 1))
	{
		if (current_count + 1 < table_items.size())
		{

			current_count = current_count + 1;
			current_item = table_items[current_count];
			OUTPUT_DEBUG(L"current_count %d/%d --> ( %d )", table_items.size(), current_count + 1, current_item.Stride);
		}
	}
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(0x30) & 1))
	{
		if (current_count > 0)
		{
			current_count = current_count - 1;
			current_item = table_items[current_count];
			OUTPUT_DEBUG(L"current_count %d/%d --> ( %d )", table_items.size(), current_count + 1, current_item.Stride);
		}
	}
	if (refresh_draw_items) 
	{
		bool exist = false;
		ImVector<DrawItem>::iterator it;
		for (it = table_items.begin(); it != table_items.end(); it++)
			if (it->Stride == mode)
			{
				exist = true;
				continue;
			}
		if (!exist) {
			id_number = id_number + 1;
			DrawItem item;
			item.ID = id_number;
			item.Stride = mode;
			table_items.push_back(item);
		}
	}
	if (find_model_type == 1 && current_count >= 0)
	{
		if (current_item.Stride == mode)
		{
			if ((GetAsyncKeyState(VK_END) & 1))
			{
				LOG_INFO("Table:Target obj is=\t(mode==%d)", mode);
			}
			matched = true;
		}
	}
	if ((radio_stride == mode && find_model_type == 2)) {
		matched = true;
	}
	if (matched && (wall_hack_type > 0 || draw_cclor_type > 0))
	{
		if (wall_hack_type >=1)
		{
			glDisable(GL_DEPTH_TEST);
		}
		
	 
		if (draw_cclor_type >= 1)
		{
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
			glEnable(GL_TEXTURE_2D);
		}
	}

	return oGLBegin(mode);
}
void impl::opengl::init()
{
	HMODULE hPENGLDLL = 0;
	do
	{
		hPENGLDLL = GetModuleHandle("opengl32.dll");
		Sleep(1000);
		LOG_INFO("GetModuleHandle with opengl32.dll..{%x}", hPENGLDLL);
	} while (!hPENGLDLL);
	Sleep(100);


	oWGLSwapBuffers = (wglSwapBuffers)GetProcAddress(hPENGLDLL, "wglSwapBuffers");
	oGLBegin = (glBegin_t)GetProcAddress(hPENGLDLL, "glBegin");


	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)oWGLSwapBuffers, (PBYTE)hkWGLSwapBuffers);
	DetourAttach(&(LPVOID&)oGLBegin, (PBYTE)hkGLBegin);
	DetourTransactionCommit();

}