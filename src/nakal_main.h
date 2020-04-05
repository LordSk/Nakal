#pragma once
#include "base.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ExplorerTab
{
	HWND hWindow = 0x0;
	DWORD processID;
};

struct Application
{
	HWND hMainWindow;
	PlainArray<ExplorerTab,128> tabs;
	bool isRunning = true;
	HANDLE hThreadExplorerScanner;

	bool Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);
	void Run();
	void OnShutdown();

	bool AddTab(const ExplorerTab& tab);
};
