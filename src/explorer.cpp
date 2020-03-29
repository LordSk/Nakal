#include "explorer.h"
#include "base.h"

#include <shellapi.h>
#include <tlhelp32.h>
#include <timeapi.h>

template<typename T, u32 CAPACITY_>
struct PlainArray
{
	enum {
		CAPACITY = CAPACITY_
	};

	T data[CAPACITY];
	i32 count = 0;

	T& Push(T elt)
	{
		ASSERT(count < CAPACITY);
		return data[count++] = elt;
	}

	void Clear()
	{
		count = 0;
	}

	inline T& operator [] (u32 index)
	{
		ASSERT(index < count);
		return data[index];
	}
};

struct EnumProcessEntry
{
	DWORD processID = 0x0;
};

static PlainArray<EnumProcessEntry,1024> g_ProcessExplorerEntries;
HWND g_foundWindow;

BOOL CALLBACK EnumWindowsProcs(HWND hWnd, LPARAM lParam)
{
	const wchar_t* targetClassName = L"CabinetWClass\0";
	wchar_t className[14];
	GetClassName(hWnd, className, 14);
	bool isValid = GetParent(hWnd) == nullptr && IsWindowVisible(hWnd) && wcscmp(targetClassName, className) == 0;

	if(isValid) {
		DWORD processID;
		GetWindowThreadProcessId(hWnd, &processID);

		for(int i = 0; i < g_ProcessExplorerEntries.count; i++) {
			if(g_ProcessExplorerEntries[i].processID == processID) {
				LOG("found explorer window (%d)", processID);
				g_foundWindow = hWnd;
				return false; // stop enum
			}
		}
	}

	return true; // continue
}

HWND OpenExplorer()
{
	// execute explorer
	SHELLEXECUTEINFO SEI = {0};
	SEI.cbSize = sizeof (SHELLEXECUTEINFO);
	SEI.fMask = SEE_MASK_NOCLOSEPROCESS;
	SEI.lpVerb = NULL;
	SEI.lpFile = L"explorer.exe"; // Open an Explorer window at the 'Computer'
	SEI.lpParameters = L","; // default path

	/*if(path != ""){
		path = path.right(path.length() - 8); // cut prefix file:///
		path.replace("/", "\\");
		path.push_front("/root,");
		wchar_t* path_w = new wchar_t[path.length() + 1];
		path.toWCharArray(path_w);
		path_w[path.length()] = L'\0';
		SEI.lpParameters = path_w;
	}*/

	SEI.lpDirectory = nullptr;
	SEI.nShow = SW_MINIMIZE; // don't popup
	SEI.hInstApp = nullptr;

	if(ShellExecuteEx(&SEI)) {
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		DWORD t0 = timeGetTime();
		while(timeGetTime() - t0 < 5000) {
			g_ProcessExplorerEntries.Clear();
			g_foundWindow = 0x0;
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

			if(!found) {
				LOG("shellExecute success but not found target process");
				continue;
			}

			if(!EnumWindows(&EnumWindowsProcs, 0x0)) {
				if(g_foundWindow) {
					return g_foundWindow;
				}
			}
		}

		// not found
		return 0x0;
	}
	else{
		LOG("ERROR: ShellExecuteEx code=%d SEI.hInstApp=%llx", GetLastError(), (i64)SEI.hInstApp);
		return 0x0;
	}
}