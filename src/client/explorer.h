#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Shell execute to open an explorer process
// find the started process
// find its window and return it
bool OpenNewExplorerTab(const char* path);
DWORD ThreadExplorerScanner(void* pData);
