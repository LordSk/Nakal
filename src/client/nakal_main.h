#pragma once
#include <common/base.h>
#include <common/logger.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ExplorerTab
{
	HWND hWindow = 0x0;
	DWORD processID;
	DWORD threadID;
	HHOOK hHook;
};

struct Application
{
	enum {
		MAX_TABS=128
	};

	bool isRunning = true;

	HWND hMainWindow;
	HANDLE hThreadExplorerScanner;

	PlainArray<ExplorerTab,MAX_TABS> tabs;

	HMODULE hHookDll;
	HOOKPROC callWndProc;
	HOOKPROC keyboardProc;

	i32 currentTabID = 0;
	u8 keyStatus[0xFF];

	bool Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);
	void Run();
	void Update();
	void Shutdown();
	void OnShutdown();

	void HandleKeyStroke(int vkey, int status);

	bool CaptureTab(ExplorerTab tab);
	void ExitTab(i32 tabID);
	void SwitchToTab(i32 tabID);

	// Shell execute to open an explorer process
	bool OpenNewExplorerTab(const char* path);
};
