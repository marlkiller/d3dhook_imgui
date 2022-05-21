#include "../global.h"
#include "opengl_impl.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../logger.h"
#include "../detours.h"

#include <exception>
#include "../imgui/imgui_impl_opengl2.h"
#include "../imgui/imgui_impl_opengl3.h"
#include <gl/GL.h>



typedef BOOL(__stdcall* wglSwapBuffers) (_In_ HDC hDc);
wglSwapBuffers oWGLSwapBuffers;

static HGLRC  g_WglContext;


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

			bool bOldOpenGL = false;

			ImGui::CreateContext();
			ImGui_ImplWin32_Init(GAME_HWND);
			//ImplementGl3();
			//gl3wInit();


			ImGui_ImplOpenGL2_Init();
			init = true;
		}


		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		DrawMainWin();

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


	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)oWGLSwapBuffers, (PBYTE)hkWGLSwapBuffers);
	//DetourAttach(&(LPVOID&)oDrawIndexed, (PBYTE)hkDrawIndexed11);
	DetourTransactionCommit();

}