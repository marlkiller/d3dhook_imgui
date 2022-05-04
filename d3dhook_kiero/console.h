#pragma once
#include <string>
#include <minwindef.h>



class ConsoleUtil {
public:
	ConsoleUtil();
	~ConsoleUtil();

	void OpenConsole();
	void CloseConsole();

private:
	FILE* mConsoleOut;
	std::streambuf* mConsoleOutBackup;
	BOOL mConsoleOpen = FALSE;
	BOOL needMenu = true;


public:
};