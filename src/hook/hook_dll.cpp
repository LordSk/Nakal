#include <common/base.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define EXPORT __declspec(dllexport)

extern "C" {
EXPORT LRESULT CALLBACK Hook(int code, WPARAM wParam, LPARAM lParam);
}

LRESULT CALLBACK Hook(int code, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	return TRUE;
}
