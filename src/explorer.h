#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ExplorerTab
{
	HWND hWindow = 0x0;
	HANDLE hProcess;
	DWORD processID;
};

// Shell execute to open an explorer process
// find the started process
// find its window and return it
bool OpenExplorer(ExplorerTab* expTab);
