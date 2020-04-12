#include "nakal_main.h"
#include "explorer.h"
#include <common/ipc.h>

#include <shellapi.h>

//#error TODO:
// - Condition variable to wake scanner thread up
// - Draw tabs on title bar (https://docs.microsoft.com/en-us/windows/win32/dwm/customframe)

// https://github.com/Dixeran/TestTab

static Application* g_App;

enum {
	WM_FOCUS_MAINWINDOW = WM_USER,

};

enum {
	KEY_PRESSED = 1,
	KEY_RELEASED = 0,
};

LRESULT CALLBACK WindowProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_CLOSE: {
			PostQuitMessage(0);
		} break;

		case WM_KEYDOWN: {
			g_App->HandleKeyStroke(wParam, KEY_PRESSED);
		} break;

		case WM_KEYUP: {
			g_App->HandleKeyStroke(wParam, KEY_RELEASED);
		} break;

		case WM_COPYDATA: {
			COPYDATASTRUCT copyData = *(COPYDATASTRUCT*)lParam;
			if(copyData.cbData == sizeof(IpcKeyStroke)) {
				const IpcKeyStroke& stroke = *(IpcKeyStroke*)copyData.lpData;
				LOG("key %d %d", stroke.vkey, stroke.status);

				g_App->HandleKeyStroke(stroke.vkey, stroke.status);
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

	memset(keyStatus, 0, sizeof(keyStatus));

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

void Application::Shutdown()
{
	isRunning = false;
	PostQuitMessage(0);
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

void Application::HandleKeyStroke(int vkey, int status)
{
	ASSERT(vkey >= 0 && vkey < ARRAY_COUNT(keyStatus));
	keyStatus[vkey] = status;

#ifdef CONF_DEBUG
	if(vkey == VK_ESCAPE) {
		Shutdown();
	}
#endif

	if(keyStatus[VK_CONTROL] && keyStatus['W']) {
		ExitTab(currentTabID);
	}
	if(keyStatus[VK_CONTROL] && keyStatus['T']) {
		OpenNewExplorerTab("");
	}
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
	const i32 tabID = tabs.Count() - 1;
	SwitchToTab(tabID);

	// Focus main window
	SendMessageA(hMainWindow, WM_USER, 0, 0);
	return true;
}

void Application::ExitTab(i32 tabID)
{
	ASSERT(tabID >= 0 && tabID < tabs.Count());

	ExplorerTab& tab = tabs[tabID];
	SendMessage(tab.hWindow, WM_QUIT, 0, 0);
	UnhookWindowsHookEx(tab.hHook);
	TerminateProcess(OpenProcess(PROCESS_TERMINATE, FALSE, tab.processID), 0);

	tabs.RemoveByIDKeepOrder(tabID);

	if(currentTabID >= tabs.Count()) {
		currentTabID--;
	}

	if(tabs.Count() == 0) {
		Shutdown();
	}
}

void Application::SwitchToTab(i32 tabID)
{
	ASSERT(tabID >= 0 && tabID < tabs.Count());

	const int tabCount = tabs.Count();
	for(int i = 0; i < tabCount; i++) {
		ExplorerTab& tab = tabs[tabID];
		if(i == tabID) {
			ShowWindow(tab.hWindow, SW_SHOW);
		}
		else {
			ShowWindow(tab.hWindow, SW_HIDE);
		}
	}

	currentTabID = tabID;
}

bool Application::OpenNewExplorerTab(const char* path)
{
	// execute explorer
	SHELLEXECUTEINFO SEI = {0};
	SEI.cbSize = sizeof(SHELLEXECUTEINFO);
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
		return true;
	}
	return false;
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
