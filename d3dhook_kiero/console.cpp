#include "pch.h"
#include "console.h"
#include <iostream>
#include <Windows.h>


using namespace std;


ConsoleUtil::ConsoleUtil() : mConsoleOut(NULL), mConsoleOutBackup(NULL) {
}

ConsoleUtil::~ConsoleUtil() {
}


void ConsoleUtil::OpenConsole() {
	if (!mConsoleOpen) {
		if (AllocConsole()) {
			mConsoleOutBackup = cout.rdbuf();
			mConsoleOut = freopen("CONOUT$", "w", stdout);
			char nt[100];
			sprintf_s(nt, "日志[%s]-%lld-%d", "d3d", GetTickCount64(), GetCurrentProcessId());
			SetConsoleTitleA(nt);
			Sleep(100);
			HWND find = FindWindowA(NULL, nt);
			if (find) {
				HMENU menu = GetSystemMenu(find, FALSE);
				if (menu) {
					if (!needMenu) {
						if (RemoveMenu(menu, 0xF060, 0)) {
							sprintf_s(nt, "日志[%s]-%lld-%d-已屏蔽关闭按钮", "d3d", GetTickCount64(), GetCurrentProcessId());
							SetConsoleTitleA(nt);
						}
					}
				}
			}
		}
		wcout.imbue(locale("", LC_CTYPE));
		mConsoleOpen = TRUE;
	}
}

void ConsoleUtil::CloseConsole() {
	if (mConsoleOpen) {
		if (mConsoleOut != NULL && mConsoleOutBackup != NULL) {
			cout.rdbuf(mConsoleOutBackup);
			fclose(mConsoleOut);
		}
		FreeConsole();
		mConsoleOpen = FALSE;
	}
}
