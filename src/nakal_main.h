#pragma once
#include "base.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ExplorerTab
{
	HWND hWindow = 0x0;
	DWORD processID;
	WNDPROC wndProc;
};

struct Application
{
	enum {
		MAX_TABS=128
	};

	HWND hMainWindow;
	PlainArray<ExplorerTab,MAX_TABS> tabs;
	bool isRunning = true;
	HANDLE hThreadExplorerScanner;

	bool Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);
	void Run();
	void Update();
	void OnShutdown();

	bool CaptureTab(ExplorerTab tab);
};
