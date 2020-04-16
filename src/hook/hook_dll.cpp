#include <common/base.h>
#include <common/ipc.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define EXPORT __declspec(dllexport)

extern "C" {
EXPORT LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam);
EXPORT LRESULT CALLBACK KeyboardProc(_In_ int code, _In_ WPARAM wParam, _In_ LPARAM lParam);
}

BOOL CALLBACK EnumWindowsProcs(HWND hWnd, LPARAM lParam);
LRESULT CALLBACK ExplorerWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define LOG(...) do { g_pHookData->SendLog(FMT(__VA_ARGS__)); } while(0)

static struct HookData* g_pHookData;

struct HookData
{
	HWND hMainClientWindow;
	HWND hExplorerWindow;
	WNDPROC OldExplorerWindowProc;

	void Init()
	{
		g_pHookData = this;
		hMainClientWindow = FindWindow(NAKAL_WND_CLASS, NULL);
		if(!hMainClientWindow) {
			LOG("ERROR: could no find main client window");
			return;
		}
		LOG("main client window = %llx", (i64)hMainClientWindow);

		EnumChildWindows(hMainClientWindow, EnumWindowsProcs, (LPARAM)this);

		if(!hExplorerWindow) {
			LOG("ERROR: could no find the explorer window");
			return;
		}
	}

	void SendMsgToMainClient(HWND explorerWindow, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if(uMsg == WM_KEYDOWN) {
			MSG msg;
			msg.message = uMsg;
			msg.wParam = wParam;
			msg.lParam = lParam;

			COPYDATASTRUCT copyData;
			copyData.dwData = GetCurrentThreadId();
			copyData.cbData = sizeof(msg);
			copyData.lpData = &msg;
			SendMessage(hMainClientWindow, WM_COPYDATA, (WPARAM)(HWND)explorerWindow, (LPARAM)(LPVOID)&copyData);
		}
	}

	void SendKeyEventToMainClient(int vkey, int flags)
	{
		// https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644984(v=vs.85)?redirectedfrom=MSDN
		IpcKeyStroke stroke;
		stroke.vkey = vkey;
		stroke.status = !((flags >> 31) & 1); // make it 1 == pressed 0 == released

		COPYDATASTRUCT copyData;
		copyData.dwData = GetCurrentThreadId();
		copyData.cbData = sizeof(stroke);
		copyData.lpData = &stroke;
		LRESULT r = SendMessage(hMainClientWindow, WM_COPYDATA, 0, (LPARAM)(LPVOID)&copyData);
		LOG("%lld", (i64)r);
	}

	void SendLog(const char* text)
	{
		IpcLogMsg msg;
		strncpy(msg.text, text, sizeof(msg.text));

		COPYDATASTRUCT copyData;
		copyData.dwData = GetCurrentThreadId();
		copyData.cbData = sizeof(msg);
		copyData.lpData = &msg;
		SendMessage(hMainClientWindow, WM_COPYDATA, 0, (LPARAM)(LPVOID)&copyData);
	}

	void CaptureExplorerWindow(HWND hWnd)
	{
		hExplorerWindow = hWnd;
		OldExplorerWindowProc = (WNDPROC)SetWindowLongPtr(hExplorerWindow, GWLP_WNDPROC, (LONG_PTR)ExplorerWindowProc);
	}
};

static HookData g_HookData;

BOOL CALLBACK EnumWindowsProcs(HWND hWnd, LPARAM lParam)
{
	HookData& app = *(HookData*)lParam;

	const wchar_t* targetClassName = L"CabinetWClass";
	wchar_t className[14];
	GetClassName(hWnd, className, 14);
	bool isValid = wcscmp(targetClassName, className) == 0;

	if(isValid) {
		DWORD processID;
		DWORD threadID = GetWindowThreadProcessId(hWnd, &processID);

		if(processID == GetCurrentProcessId()) {
			LOG("window %ls windowProcessID=%d dllProcessID=%d", className, processID, GetCurrentProcessId());
			app.CaptureExplorerWindow(hWnd);
			return false;
		}
	}

	return true; // continue
}

LRESULT CALLBACK ExplorerWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LOG("ExplorerWindowProc: %d", msg);

	switch(msg) {
		case WM_NCCALCSIZE : {
			if(wParam == TRUE) {
				// NOTE: when we increase this we can more easily resize
				NCCALCSIZE_PARAMS *pncsp = (NCCALCSIZE_PARAMS*)lParam;
				pncsp->rgrc[0].left   = pncsp->rgrc[0].left   + 0; // keep 1px border
				pncsp->rgrc[0].top    = pncsp->rgrc[0].top    + 0;
				pncsp->rgrc[0].right  = pncsp->rgrc[0].right  - 0;
				pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 0;
				return 0;
			}
		} break;
	}

	return g_HookData.OldExplorerWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode < 0) return CallNextHookEx(0, nCode, wParam, lParam);

	const CWPSTRUCT& cwp = *(CWPSTRUCT*)lParam;
	g_HookData.SendMsgToMainClient(cwp.hwnd, cwp.message, cwp.wParam, cwp.lParam);
	return CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	if(nCode < 0) return CallNextHookEx(0, nCode, wParam, lParam);

	g_HookData.SendKeyEventToMainClient(wParam, lParam);
	return CallNextHookEx(0, nCode, wParam, lParam);
}

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH) {
		static bool init = false;
		if(!init) {
			init = true;
			g_HookData.Init();
		}
	}

	return TRUE;
}
