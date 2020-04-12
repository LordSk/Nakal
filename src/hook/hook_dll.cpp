#include <common/base.h>
#include <common/logger.h>
#include <common/ipc.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define EXPORT __declspec(dllexport)

extern "C" {
EXPORT LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam);
EXPORT LRESULT CALLBACK KeyboardProc(_In_ int code, _In_ WPARAM wParam, _In_ LPARAM lParam);
}

struct HookData
{
	HWND hMainClientWindow;

	void Init()
	{
		hMainClientWindow = FindWindow(NAKAL_WND_CLASS, NULL);
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
		SendMessage(hMainClientWindow, WM_COPYDATA, 0, (LPARAM)(LPVOID)&copyData);
	}
};

static HookData g_HookData;

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	/*if(nCode < 0) return CallNextHookEx(0, nCode, wParam, lParam);

	const CWPSTRUCT& cwp = *(CWPSTRUCT*)lParam;
	g_HookData.SendMsgToMainClient(cwp.hwnd, cwp.message, cwp.wParam, cwp.lParam);*/
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
		g_HookData.Init();
	}

	return TRUE;
}
