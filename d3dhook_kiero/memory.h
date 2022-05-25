#pragma once
#include <wtypes.h>

// 读取内存4个字节
inline DWORD ReadMemory_DWORD(HANDLE hProcess,DWORD dwReadAddr,BOOL *bStatus)
{
	DWORD dwMemory = 0;
	BOOL fok;
	fok = ReadProcessMemory(hProcess,
		(LPCVOID)dwReadAddr,
		&dwMemory,
		4,
		NULL);
	if (bStatus != 0)
		*bStatus = fok;
	return dwMemory;
}

// 读取内存一个字节
inline BYTE ReadMemory_BYTE(HANDLE hProcess, DWORD dwReadAddr, BOOL *bStatus)
{
	BYTE dwMemory = 0; 
	BOOL fok;
	fok = ReadProcessMemory(hProcess,
		(LPCVOID)dwReadAddr,
		&dwMemory,
		1,
		NULL);
	if (bStatus != 0)
		*bStatus = fok;
	return dwMemory;
}

inline float ReadMemory_float(HANDLE hProcess, DWORD dwReadAddr, BOOL *bStatus)
{
	float fMemory = 0;
	BOOL fok;
	fok = ReadProcessMemory(hProcess,
		(LPCVOID)dwReadAddr,
		&fMemory,
		4,
		NULL);
	if (bStatus != 0)
		*bStatus = fok;
	return fMemory;
}

inline BOOL WriteMemory_DWORD(HANDLE hProcess, DWORD dwReadAddr, DWORD dwNum)
{
	return WriteProcessMemory(hProcess,
		(LPVOID)dwReadAddr,
		&dwNum,
		4,
		NULL
	);
}

inline BOOL WriteMemory_float(HANDLE hProcess, DWORD dwReadAddr, float fNum)
{
	return WriteProcessMemory(hProcess,
		(LPVOID)dwReadAddr,
		&fNum,
		4,
		NULL
	);
}

