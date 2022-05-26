#include "../imgui_constants.h"
#include "opengl_impl.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../logger.h"
#include "../detours.h"

#include <exception>
#include "../imgui/imgui_impl_opengl2.h"
//#include "../imgui/imgui_impl_opengl3.h"
#include <string>
#include "../imgui/soil.h"
#include "../imgui/GL.h"
#include "../imgui/glcorearb.h"
#include "../common_utils.h"
#include "../imgui_draw_util.h"
#define PI 3.1415926



typedef BOOL(APIENTRY* wglSwapBuffers)(HDC  hdc);
typedef void (APIENTRY* glBegin_t)(GLenum mode);
typedef void (APIENTRY* glClear_t)(GLbitfield mask);
typedef void (APIENTRY* glColor4f_t)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
//typedef void (APIENTRY* glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
//typedef void (APIENTRY* glVertex3f)(GLfloat x, GLfloat y, GLfloat z);


wglSwapBuffers oWGLSwapBuffers;
glBegin_t oGLBegin;
glClear_t oGLClear;
glColor4f_t oGLColor4f;
GLuint texture_id[1024];



void LoadTextureFile(int index, const char* image)
{
	int width, height;
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &texture_id[index]);
	glBindTexture(GL_TEXTURE_2D, texture_id[index]);
	//unsigned char* soilimage = SOIL_load_image_from_memory(image, size, &width, &height, 0, SOIL_LOAD_RGBA);
	unsigned char* soilimage = SOIL_load_image(image, &width, &height, 0, SOIL_LOAD_RGBA);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // GL_REPEAT：当纹理比表面小时重复使用纹理以填满每个点。GL_CLAMP：比1大的当作1，比0小的当作0。bilinear：二次插值，精度更高，但需要自己动手计算
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // GL_NEAREST：取比较接近的那个像素。GL_LINEAR：以周围四个像素的平均值做为纹理。bilinear：二次插值，精度更高，但需要自己动手计算
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, soilimage); // 使用glTexImage2D()时所采用的位图文件分辨率必须为：64×64、128×128、256×256三种格式，如果其他大小则会出现绘制不正常。
	SOIL_free_image_data(soilimage);
	glBindTexture(GL_TEXTURE_2D, last_texture);
}
void LoadTextureMemary(int index, int resource_id, char* resource_type)
{
	int width, height;
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &texture_id[index]);
	glBindTexture(GL_TEXTURE_2D, texture_id[index]);



	HINSTANCE hInst = GetModuleHandle("d3dhook_kiero.dll");
	HRSRC res = ::FindResource(hInst, MAKEINTRESOURCE(resource_id), TEXT(resource_type));
	DWORD size = ::SizeofResource(hInst, res);
	HGLOBAL mem = ::LoadResource(hInst, res);
	LPVOID raw_data = ::LockResource(mem);
	OUTPUT_DEBUG(L"read_resource > res {%d}, size {%d} , mem {%d}", res, size, mem);
	unsigned char* soilimage = SOIL_load_image_from_memory((unsigned char*)(raw_data), size, &width, &height, 0, SOIL_LOAD_RGBA);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, soilimage);
	SOIL_free_image_data(soilimage);
	glBindTexture(GL_TEXTURE_2D, last_texture);
}
	

void APIENTRY hkGLColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	(*oGLColor4f)(red, green, blue, alpha);
}


static float depth_range_zNear = 1;
static float depth_range_zFar = 1;
static bool enable_depth_dev = false;
static bool enable_draw_esp = false;
static bool open_dev = false;
HANDLE g_hProcess;
DWORD cstrike_base;

const DWORD el_num_base = 0x620FCC; //cstrike.exe + 620FCC          //游戏中除了自己其他人人数
const DWORD Control_CursorAngle_X_offset = 0x19E10C8;			//鼠标x角度 水平朝向
const DWORD Control_CursorAngle_Y_offset = 0x19E10C4;			//鼠标y角度 高低朝向
const DWORD Control_CursorAngle_FOV_X = 0x195fe58;			//鼠标x角度 水平朝向
const DWORD Control_CursorAngle_FOV_Y = 0x195fe5C;			//鼠标y角度 高低朝向
const DWORD Control_CursorAngle_FOV_Z = 0x195fe60;			//鼠标y角度 高低朝向

typedef struct PlayerData
{
	float position[3];
	float hp;
}PlayerData;

RECT  g_winRect = { 0 };

bool WorldToScreen(float position[3], float screen[2], float matrix[16], int windowWidth, int windowHeight)
{
    //Matrix-vector Product, multiplying world(eye) coordinates by projection matrix = clipCoords
	float clipCoords[4];
	clipCoords[0] = position[0] * matrix[0] + position[1] * matrix[4] + position[2] * matrix[8] + matrix[12];
	clipCoords[1] = position[0] * matrix[1] + position[1] * matrix[5] + position[2] * matrix[9] + matrix[13];
	clipCoords[2] = position[0] * matrix[2] + position[1] * matrix[6] + position[2] * matrix[10] + matrix[14];
	clipCoords[3] = position[0] * matrix[3] + position[1] * matrix[7] + position[2] * matrix[11] + matrix[15];

	if (clipCoords[3] < 0.1f)
		return false;

	//perspective division, dividing by clip.W = Normalized Device Coordinates
	float NDC[3];
	NDC[0] = clipCoords[0] / clipCoords[3];
	NDC[1] = clipCoords[1] / clipCoords[3];
	NDC[2] = clipCoords[2] / clipCoords[3];

	screen[0] = (windowWidth / 2 * NDC[0]) + (NDC[0] + windowWidth / 2);
	screen[1] = -(windowHeight / 2 * NDC[1]) + (NDC[1] + windowHeight / 2);
    return true;
}
void LoadGameInfo() {

	g_hProcess = GetCurrentProcess();
	cstrike_base = (uintptr_t)GetModuleHandle("cstrike.exe");
	LOG_INFO("g_hProcess -> %x, cstrike.exe -> %x", g_hProcess, cstrike_base);

}
void ReadDataList(int index, PlayerData* data)
{
	//[[[cstrike.exe + 11069BC]+ 7C + (i*324)] + 4] + 8
	DWORD addr;
	ReadProcessMemory(g_hProcess, (PBYTE*)(cstrike_base + 0x11069BC), &addr, sizeof(DWORD), 0);
	ReadProcessMemory(g_hProcess, (PBYTE*)(addr + 0x7C + (index * 0x324)), &addr, sizeof(DWORD), 0);//其他人的基址
	ReadProcessMemory(g_hProcess, (PBYTE*)(addr + 0x4), &addr, sizeof(DWORD), 0);//其他人2级基址
	ReadProcessMemory(g_hProcess, (PBYTE*)(addr + 0x8), &data->position, sizeof(float[3]), 0);//其他人位置
	ReadProcessMemory(g_hProcess, (PBYTE*)(addr + 0x160), &data->hp, sizeof(float), 0);//其他人血量

}

void DrawOpenGLDIY() {


	if (p_open)
	{
		ImGui::Begin("OpenGLDraw");
		ImGui::Checkbox("DrawESP", &enable_draw_esp);
		ImGui::End();
	}

	if (!enable_draw_esp)
		return;
	PlayerData my_data = { 0 };
	ReadDataList(0, &my_data);
	
	if (my_data.hp <= 1 || my_data.hp > 100) // FIXME Alive
		return;

	int numb = 0;
	int FOV;
	memcpy(&numb, (PBYTE*)(cstrike_base + el_num_base), sizeof(numb));
	float CursorAngle_X, CursorAngle_Y, CursorAngle_FOV_X, CursorAngle_FOV_Y, CursorAngle_FOV_Z;
	memcpy(&FOV, (PBYTE*)(cstrike_base + 0x617AC8), 4);
	ReadProcessMemory(g_hProcess, (PBYTE*)(cstrike_base + Control_CursorAngle_X_offset), &CursorAngle_X, sizeof(CursorAngle_X), 0);
	ReadProcessMemory(g_hProcess, (PBYTE*)(cstrike_base + Control_CursorAngle_Y_offset), &CursorAngle_Y, sizeof(CursorAngle_Y), 0);
	ReadProcessMemory(g_hProcess, (PBYTE*)(cstrike_base + Control_CursorAngle_FOV_X), &CursorAngle_FOV_X, sizeof(CursorAngle_FOV_X), 0);
	ReadProcessMemory(g_hProcess, (PBYTE*)(cstrike_base + Control_CursorAngle_FOV_Y), &CursorAngle_FOV_Y, sizeof(CursorAngle_FOV_Y), 0);
	ReadProcessMemory(g_hProcess, (PBYTE*)(cstrike_base + Control_CursorAngle_FOV_Z), &CursorAngle_FOV_Z, sizeof(CursorAngle_FOV_Z), 0);

	//OUTPUT_DEBUG(L"my_data > %f,%f,%f,%f,%f,%f", my_data.hp, CursorAngle_X, CursorAngle_Y, CursorAngle_FOV_X, CursorAngle_FOV_Y, CursorAngle_FOV_Z);

	// TODO support Matrix
	for (int i = 1; i <= numb; i++)
	{
 		PlayerData el_data = { 0 };
		ReadDataList(i, &el_data);
		float target_face_x, target_face_y, diff_face_x, diff_face_y;

		if (el_data.position[0] > CursorAngle_FOV_X && el_data.position[1] >= CursorAngle_FOV_Y)//第一象限
			target_face_x = (FLOAT)((double)atan2(el_data.position[1] - CursorAngle_FOV_Y, el_data.position[0] - CursorAngle_FOV_X) * 180 / PI);
		else if (el_data.position[0] <= CursorAngle_FOV_X && el_data.position[1] > CursorAngle_FOV_Y)//第二象限
			target_face_x = 180 - (FLOAT)((double)atan2(el_data.position[1] - CursorAngle_FOV_Y, CursorAngle_FOV_X - el_data.position[0]) * 180 / PI);
		else if (el_data.position[0] < CursorAngle_FOV_X && el_data.position[1] <= CursorAngle_FOV_Y)//第三象限
			target_face_x = 180 + (FLOAT)((double)atan2(CursorAngle_FOV_Y - el_data.position[1], CursorAngle_FOV_X - el_data.position[0]) * 180 / PI);
		else if (el_data.position[0] >= CursorAngle_FOV_X && el_data.position[1] < CursorAngle_FOV_Y)//第四象限
			target_face_x = 360 - (FLOAT)((double)atan2(CursorAngle_FOV_Y - el_data.position[1], el_data.position[0] - CursorAngle_FOV_X) * 180 / PI);
		else 
			continue;
		float distance = sqrt((el_data.position[0] - CursorAngle_FOV_X) * (el_data.position[0] - CursorAngle_FOV_X) + (CursorAngle_FOV_Y - el_data.position[1]) * (CursorAngle_FOV_Y - el_data.position[1]));
		float distance_3d = sqrt(pow(my_data.position[0] - el_data.position[0], 2) + pow(my_data.position[1] - el_data.position[1], 2) + pow(my_data.position[2] - el_data.position[2], 2));
		if (el_data.position[2] > CursorAngle_FOV_Z)//上方
			target_face_y = (FLOAT)(-(double)atan2(el_data.position[2] - CursorAngle_FOV_Z, distance) * 180 / PI);
		else if (el_data.position[2] < CursorAngle_FOV_Z)//下方
			target_face_y = (FLOAT)((double)atan2(CursorAngle_FOV_Z - el_data.position[2], distance) * 180 / PI);
		else
			continue;

		diff_face_x = CursorAngle_X - target_face_x;
		if (diff_face_x <= -180)//跨0轴的两种情况
			diff_face_x += 360;
		if (diff_face_x >= 180)
			diff_face_x -= 360;

		diff_face_y = target_face_y - CursorAngle_Y;


		FLOAT cal_fov_y = (FLOAT)((double)atan2(HWND_SCREEN_Y, HWND_SCREEN_X) * 180 / PI);
		if (fabs(diff_face_x) > 45 || fabs(diff_face_y) > cal_fov_y)
			continue;// 不在屏幕范围内


		// FIXME FOV
		float offset = 1000 / distance;
		float screen_x, screen_y;
		int diff_x = (int)(tan(diff_face_x * PI / 180) * ((HWND_SCREEN_X) / 2));
		screen_x = (float)(HWND_SCREEN_X / 2 + diff_x);
		int diff_y = (int)(tan(diff_face_y * PI / 180) * ((HWND_SCREEN_X) / 2));
		screen_y = (float)(HWND_SCREEN_Y / 2 + diff_y);
		screen_x = screen_x - 8;
		screen_y = screen_y - 7; 

		//float diff_x = my_data.position[0] - el_data.position[0];
		//float diff_y = my_data.position[1] - el_data.position[1];
		//float diff_z = my_data.position[2] - el_data.position[2];
		//float distance_2d = sqrt(pow(diff_x, 2) + pow(diff_y, 2));
		//float instance_3d = sqrt(pow(diff_x, 2) + pow(diff_y, 2) + pow(diff_z, 2));

		//float offset = 1000 / distance_3d;
		//float tmp = tan(diff_face_x * PI / 180);
		//float screen_x = HWND_SCREEN_X / 2 + (tmp)*HWND_SCREEN_X / 2 - 8 * offset * 90 / FOV;
		//float tmp2 = tan(-CursorAngle_Y * PI / 180) * distance_2d + diff_z; // AngleMouse 鼠标角度加个负号，要取相反值,不加负号会造成方框向抬起的角度上移
		//float screen_y = HWND_SCREEN_Y / 2 + tmp2 / distance_2d * 512 * 90 / FOV - 5;

		float w = 16.666666 * offset * 90 / FOV;
		float h = 33.666666 * offset * 90 / FOV;
		DrawEspBox(box_type, screen_x, screen_y, w, h, 255, 255, 255, 255);
	}
	
}
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
			//LoadTextureFile(1,"C:\\Users\\voidm\\Desktop\\111.jpg"); 
			LoadTextureMemary(1,IDB_PNG1,"PNG"); // RED
			LoadTextureMemary(2,IDB_PNG2,"PNG"); // GREEN
			LoadTextureMemary(3, IDB_BITMAP1,"JPG"); // DIY

			LoadGameInfo();
 			init = true;
		}


		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		DrawMainWin();
		DrawOpenGLDIY();

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

	if (radio_stride == mode && find_model_type == 2){
		matched = true;
	}
	if (matched && (wall_hack_type > 0 || draw_cclor_type > 0))
	{

		if (enable_depth_dev)
		{
			glDepthRange(depth_range_zNear, depth_range_zFar);
		}

		if (wall_hack_type ==1)
		{
			// https://blog.csdn.net/qq_43872529/article/details/102496602
			glDisable(GL_DEPTH_TEST);
			/*if (Player || Weapon)
				glDepthRange(0, 0.5);
			else
				glDepthRange(0.5, 1);*/
		}
		
		if (draw_cclor_type != 2)
		{
			//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // close
			// GL_LINE , Must be work with hkGLClear open
			/*glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(3.0);
			glColor3f(255, 255, 255);*/
		}
		
		if (draw_cclor_type == 1)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			glBindTexture(GL_TEXTURE_2D, texture_id[1]);
		}else if (draw_cclor_type ==2)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			glBindTexture(GL_TEXTURE_2D, texture_id[3]);
			/*glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			glBindTexture(GL_TEXTURE_2D, texture_id[2]);*/


			//如果要使用透明度, 则需要启用混合:glEnable(GL_BLEND);
			// RE DRAW
			/*const GLfloat squareVertices[] = {
			0.5, 0.5, 0.0,
			-0.5, 0.5, 0.0,
			0.5, -0.5, 0.0,
			-0.5, -0.5, 0.0 };
			glEnableClientState(GL_VERTEX_ARRAY);
			glColor4f(1.0, 0.0, 0.0, 0.5);
			glLoadIdentity();
			glTranslatef(0, 0, -5);
			glVertexPointer(3, GL_FLOAT, 0, squareVertices);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);*/

			// FIXME 
			/*glEnable(GL_TEXTURE_2D);
			glColor4f(1.0, 0.0, 0.0, 0.5);
			glDisable(GL_TEXTURE_2D);*/
		}
		
	}

	return oGLBegin(mode);
}

void APIENTRY hkGLClear(GLbitfield mask)
{
	if (mask == GL_DEPTH_BUFFER_BIT)
	{
		//mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		//(*oGLClear)(GL_COLOR_BUFFER_BIT), glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	return oGLClear(mask);

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
	oGLClear = (glClear_t)GetProcAddress(hPENGLDLL, "glClear");
	oGLColor4f = (glColor4f_t)GetProcAddress(hPENGLDLL, "glColor4f");


	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)oWGLSwapBuffers, (PBYTE)hkWGLSwapBuffers);
	DetourAttach(&(LPVOID&)oGLBegin, (PBYTE)hkGLBegin);
	DetourAttach(&(LPVOID&)oGLClear, (PBYTE)hkGLClear);
	DetourAttach(&(LPVOID&)oGLColor4f, (PBYTE)hkGLColor4f);
	DetourTransactionCommit();

}