#include "nakal_main.h"
#include "explorer.h"
#include <common/ipc.h>

//#error TODO:
// - New tab, Exit tab (key shortcuts for now)
// - Allow tab switching (key shortcuts for now)
// - Draw tabs on title bar (https://docs.microsoft.com/en-us/windows/win32/dwm/customframe)

// https://github.com/Dixeran/TestTab

static Application* g_App;

enum {
	WM_FOCUS_MAINWINDOW = WM_USER,

};

LRESULT CALLBACK WindowProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_CLOSE: {
			PostQuitMessage(0);
		} break;

		case WM_KEYDOWN: {
			if(wParam == VK_ESCAPE) {
				PostQuitMessage(0);
			}
		} break;

		case WM_COPYDATA: {
			COPYDATASTRUCT copyData = *(COPYDATASTRUCT*)lParam;
			if(copyData.cbData == sizeof(MSG)) {
				MSG& msg = *(MSG*)copyData.lpData;
				LOG("message %u", msg.message);

				if(msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
					PostQuitMessage(0);
				}
			}
			if(copyData.cbData == sizeof(IpcKeyStroke)) {
				const IpcKeyStroke& stroke = *(IpcKeyStroke*)copyData.lpData;
				LOG("key %d %d", stroke.vkey, stroke.status);

				if(stroke.vkey == VK_ESCAPE) {
					PostQuitMessage(0);
				}
			}
		} break;

		case WM_FOCUS_MAINWINDOW: {
			SetActiveWindow(hWindow);
		} break;
	}

	return DefWindowProc(hWindow, uMsg, wParam, lParam);
}

bool Application::Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Register the window class.
	WNDCLASS wc = {};
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = NAKAL_WND_CLASS;
	RegisterClass(&wc);

	// Create the window.
	hMainWindow = CreateWindowEx(
		0,                              // Optional window styles.
		NAKAL_WND_CLASS,                     // Window class
		L"Learn to Program Windows",    // Window text
		WS_OVERLAPPEDWINDOW,            // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
		);

	if(hMainWindow == NULL)
	{
		LOG("Error creating window: %x", GetLastError());
		return false;
	}

	ShowWindow(hMainWindow, nCmdShow);

	hThreadExplorerScanner = CreateThread(NULL, 0, ThreadExplorerScanner, this, 0, NULL);


	hHookDll = LoadLibraryA("HookDll_debug.dll");
	if(!hHookDll) {
		LOG("Error: loading hook dll (%d)", GetLastError());
		return false;
	}

	callWndProc = (HOOKPROC)GetProcAddress(hHookDll, "CallWndProc");
	if(!callWndProc) {
		LOG("Error: getting CallWndProc() procedure address (%d)", GetLastError());
		return false;
	}

	keyboardProc = (HOOKPROC)GetProcAddress(hHookDll, "KeyboardProc");
	if(!keyboardProc) {
		LOG("Error: getting KeyboardProc() procedure address (%d)", GetLastError());
		return false;
	}

	OpenNewExplorerTab("");
	return true;
}

void Application::Run()
{
	MSG msg = { };
	while(GetMessageA(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
		Update();
	}

	OnShutdown();
}

void Application::Update()
{
}

void Application::OnShutdown()
{
	LOG("Exiting...");

	isRunning = false;

	WaitForSingleObject(hThreadExplorerScanner, 2000);

	for(int i = 0; i < tabs.Count(); i++) {
		UnhookWindowsHookEx(tabs[i].hHook);
		TerminateProcess(OpenProcess(PROCESS_TERMINATE, FALSE, tabs[i].processID), 0);
	}
	tabs.Clear();
}

bool Application::CaptureTab(ExplorerTab tab)
{
	if(tabs.IsFull()) {
		return false;
	}


	RECT mainWindowRect;
	GetWindowRect(hMainWindow, &mainWindowRect);

	const int width = mainWindowRect.right - mainWindowRect.left;
	const int height = mainWindowRect.bottom - mainWindowRect.top;

	HWND hExplorerWnd = tab.hWindow;

	// Explorer window:
	// Set parent to our main window
	// Remove explorer window title and border
	// Set as a child (WS_CHILD)
	// Move it related to the main window position
	if(hExplorerWnd) {
		SetParent(hExplorerWnd, hMainWindow);
		LONG_PTR style = GetWindowLongPtr(hExplorerWnd, GWL_STYLE);
		style = style & (~WS_BORDER) & (~WS_SYSMENU) & (~WS_CAPTION) & (~WS_SIZEBOX);
		style |= WS_CHILD;
		LONG_PTR r = SetWindowLongPtr(hExplorerWnd, GWL_STYLE, style);
		if(!r) {
			LOG("Error: could not set tab style (%d)", GetLastError());
			return false;
		}
		MoveWindow(hExplorerWnd, 0, -31, width - 14, height - 10, true);
	}

	// hooks to get messages
	/*tab.hHook = SetWindowsHookEx(WH_CALLWNDPROC, callWndProc, hHookDll, tab.threadID);
	if(tab.hHook == NULL) {
		LOG("Error: failed to install tab callwnd hook (%d)", GetLastError());
		return false;
	}*/
	tab.hHook = SetWindowsHookEx(WH_KEYBOARD, keyboardProc, hHookDll, tab.threadID);
	if(tab.hHook == NULL) {
		LOG("Error: failed to install tab keyboard hook (%d)", GetLastError());
		return false;
	}

	tabs.Push(tab);

	// Focus main window
	SendMessageA(hMainWindow, WM_USER, 0, 0);
	return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	LogInit("nakal.log");
	LOG(".: Nakal :.");

	Application app;
	g_App = &app;
	if(!app.Init(hInstance, hPrevInstance, pCmdLine, nCmdShow)) {
		LOG("Error initialising application");
		return 1;
	}

	app.Run();
	return 0;
}
