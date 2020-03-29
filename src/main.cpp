#include "base.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "explorer.h"

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
	}

	return DefWindowProc(hWindow, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	LOG("hello tabs!");

	// Register the window class.
	const wchar_t CLASS_NAME[]  = L"Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// Create the window.
	HWND hMainWindow = CreateWindowEx(
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
		LOG("%x", GetLastError());
		return 0;
	}

	ShowWindow(hMainWindow, nCmdShow);

	RECT mainWindowRect;
	GetWindowRect(hMainWindow, &mainWindowRect);

	const int width = mainWindowRect.right - mainWindowRect.left;
	const int height = mainWindowRect.bottom - mainWindowRect.top;

	ExplorerTab expTab;
	OpenExplorer(&expTab);
	HWND hExplorerWnd = expTab.hWindow;

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
		SetWindowLongPtr(hExplorerWnd, GWL_STYLE, style);
		MoveWindow(hExplorerWnd, 0, -31, width - 14, height - 10, true); // must
	}

	// Focus main window
	SetActiveWindow(hMainWindow);

	// Copy title from explorer window
	wchar_t title[1024];
	GetWindowText(hExplorerWnd, title, ARRAY_COUNT(title));
	SetWindowText(hMainWindow, title);

	// Copy icon from explorer window
	HICON hIcon = (HICON)SendMessage(hExplorerWnd, WM_GETICON, ICON_SMALL, 0);
	SendMessage(hMainWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	MSG msg = { };
	while(GetMessageA(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	TerminateProcess(OpenProcess(PROCESS_TERMINATE, FALSE, expTab.processID), 0);

	LOG("Exiting...");

	return 0;
}
