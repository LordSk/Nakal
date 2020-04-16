#include "nakal_main.h"
#include "explorer.h"
#include <common/ipc.h>

#include <shellapi.h>
#include <dwmapi.h>
#include <windowsx.h>

//#error TODO:
// - Fix opening a explorer window make the title bar unsable (fix margin for it as well?)
// - Fix resizing on borders being hard when we handle the WM_NCCALCSIZE message
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

LRESULT HitTestNCA(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	const int TOPEXTENDWIDTH = 31;
	const int BOTTOMEXTENDWIDTH = 0;
	const int LEFTEXTENDWIDTH = 0;
	const int RIGHTEXTENDWIDTH = 0;

	// Get the point coordinates for the hit test.
	POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	// Get the window rectangle.
	RECT rcWindow;
	GetWindowRect(hWnd, &rcWindow);

	// Get the frame rectangle, adjusted for the style without a caption.
	RECT rcFrame = { 0 };
	AdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL);

	// Determine if the hit test is for resizing. Default middle (1,1).
	USHORT uRow = 1;
	USHORT uCol = 1;
	bool fOnResizeBorder = false;

	// Determine if the point is at the top or bottom of the window.
	if(ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + TOPEXTENDWIDTH) {
		fOnResizeBorder = (ptMouse.y < (rcWindow.top - rcFrame.top));
		uRow = 0;
	}
	else if(ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - BOTTOMEXTENDWIDTH) {
		uRow = 2;
	}

	// Determine if the point is at the left or right of the window.
	if(ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + LEFTEXTENDWIDTH) {
		uCol = 0; // left side
	}
	else if(ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - RIGHTEXTENDWIDTH) {
		uCol = 2; // right side
	}

	// Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
	LRESULT hitTests[3][3] =
	{
		{ HTTOPLEFT,    fOnResizeBorder ? HTTOP : HTCAPTION,    HTTOPRIGHT },
		{ HTLEFT,       HTNOWHERE,     HTRIGHT },
		{ HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT },
	};

	return hitTests[uRow][uCol];
}

LRESULT CALLBACK WindowProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	if(DwmDefWindowProc(hWindow, uMsg, wParam, lParam, &ret)) {
		return ret;
	}

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
			if(copyData.dwData == GetCurrentThreadId()) break;

			const IpcHeader& header = *(IpcHeader*)copyData.lpData;
			//LOG("header = %d", header.id);

			if(header.id == IPCMT_KEY_STROKE && copyData.cbData == sizeof(IpcKeyStroke)) {
				const IpcKeyStroke& stroke = *(IpcKeyStroke*)copyData.lpData;
				LOG("key %d %d", stroke.vkey, stroke.status);

				g_App->HandleKeyStroke(stroke.vkey, stroke.status);
			}
			else if(header.id == IPCMT_LOG_MSG && copyData.cbData == sizeof(IpcLogMsg)) {
				const IpcLogMsg& log = *(IpcLogMsg*)copyData.lpData;
				LOG("DLL#%llx> %s", copyData.dwData, log.text);
			}
		} break;

		case WM_ACTIVATE: {
			MARGINS margins;
			margins.cxLeftWidth = 0;
			margins.cxRightWidth = 0;
			margins.cyBottomHeight = 0;
			margins.cyTopHeight = 31;

			HRESULT hr = DwmExtendFrameIntoClientArea(hWindow, &margins);
			if(!SUCCEEDED(hr)) {
				LOG("Error: extending frame (%d)", GetLastError());
				return -1;
			}
		} break;

		case WM_NCCALCSIZE : {
			if(wParam == TRUE) {
				// NOTE: when we increase this we can more easily resize
				NCCALCSIZE_PARAMS *pncsp = (NCCALCSIZE_PARAMS*)lParam;
				pncsp->rgrc[0].left   = pncsp->rgrc[0].left   + 10; // keep 1px border
				pncsp->rgrc[0].top    = pncsp->rgrc[0].top    + 0;
				pncsp->rgrc[0].right  = pncsp->rgrc[0].right  - 10;
				pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 10;
				return 0;
			}
		} break;

		case WM_CREATE: {
			RECT rcClient;
			GetWindowRect(hWindow, &rcClient);

			// Inform the application of the frame change.
			SetWindowPos(hWindow,
						 NULL,
						 rcClient.left, rcClient.top,
						 rcClient.right-rcClient.left, rcClient.bottom-rcClient.top,
						 SWP_FRAMECHANGED);
		} break;


		case WM_NCHITTEST: {
			LRESULT ret = HitTestNCA(hWindow, wParam, lParam);
			if(ret != HTNOWHERE) {
				return ret;
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
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	RegisterClass(&wc);

	// Create the window.
	hMainWindow = CreateWindowEx(
		0,                              // Optional window styles.
		NAKAL_WND_CLASS,                     // Window class
		L"Nakal",    // Window text
		WS_OVERLAPPEDWINDOW,            // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
		);

	if(hMainWindow == NULL) {
		LOG("Error creating window (%d)", GetLastError());
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

	//OpenNewExplorerTab("");
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
		MoveWindow(hExplorerWnd, 0, 0, width - 1, height - 1, true);
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
