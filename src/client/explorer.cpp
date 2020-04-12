#include "explorer.h"
#include "nakal_main.h"

#include <tlhelp32.h>
#include <timeapi.h>

struct EnumProcessEntry
{
	DWORD processID = 0x0;
};

static PlainArray<EnumProcessEntry,1024> g_ProcessExplorerEntries;

BOOL CALLBACK EnumWindowsProcs(HWND hWnd, LPARAM lParam)
{
	Application& app = *(Application*)lParam;

	const wchar_t* targetClassName = L"CabinetWClass\0";
	wchar_t className[14];
	GetClassName(hWnd, className, 14);
	bool isValid = GetParent(hWnd) == nullptr && IsWindowVisible(hWnd) && wcscmp(targetClassName, className) == 0;

	if(isValid) {
		DWORD processID;
		DWORD threadID = GetWindowThreadProcessId(hWnd, &processID);

		for(int i = 0; i < g_ProcessExplorerEntries.count; i++) {
			if(g_ProcessExplorerEntries[i].processID == processID) {
				LOG("found explorer window (%d)", processID);

				ExplorerTab tab;
				tab.hWindow = hWnd;
				tab.processID = processID;
				tab.threadID = threadID;
				app.CaptureTab(tab);
				return false; // stop enum
			}
		}
	}

	return true; // continue
}

DWORD ThreadExplorerScanner(void* pData)
{
	Application& app = *(Application*)pData;

	DWORD t0 = timeGetTime();
	while(app.isRunning)
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		g_ProcessExplorerEntries.Clear();
		bool found = false;

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if(Process32First(snapshot, &entry)) {
			while(Process32Next(snapshot, &entry)) {
				if(wcscmp(entry.szExeFile, L"explorer.exe") == 0) {
					found = true;
					EnumProcessEntry savedEntry;
					savedEntry.processID = entry.th32ProcessID;
					g_ProcessExplorerEntries.Push(savedEntry);
				}
			}
		}

		while(!EnumWindows(&EnumWindowsProcs, (LPARAM)&app));

		Sleep(100);
	}

	return 0;
}
