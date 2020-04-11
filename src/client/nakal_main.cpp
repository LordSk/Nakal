#include "nakal_main.h"
#include "explorer.h"

//#error TODO:
// - Pass child window messages to parent SetWindowLongPtr (https://docs.microsoft.com/en-gb/windows/win32/winmsg/using-window-procedures?redirectedfrom=MSDN#subclassing_window)
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

		case WM_FOCUS_MAINWINDOW: {
			SetActiveWindow(hWindow);
		} break;
	}

	return DefWindowProc(hWindow, uMsg, wParam, lParam);
}

bool Application::Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Register the window class.
	const wchar_t CLASS_NAME[]  = L"Nakal";
	WNDCLASS wc = {};
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	// Create the window.
	hMainWindow = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
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


	HMODULE hHookDll = LoadLibraryA("HookDll_debug.dll");
	if(!hHookDll) {
		LOG("Error: loading hook dll (%d)", GetLastError());
		return false;
	}

	HOOKPROC hookProc = (HOOKPROC)GetProcAddress(hHookDll, "Hook");
	if(!hookProc) {
		LOG("Error: getting hook procedure adress (%d)", GetLastError());
		return false;
	}

	HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, hookProc, hHookDll, 0);
	if(hook == NULL) {
		LOG("Error: failed to install tab hook (%d)", GetLastError());
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

	ExplorerTab& newTab = tabs.Push(tab);

	// Focus main window
	SendMessageA(hMainWindow, WM_USER, 0, 0);
	return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
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
