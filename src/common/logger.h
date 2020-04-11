#pragma once
#include "base.h"

struct Logger
{
	FILE* file = nullptr;

	void Init(const char* filename);
	void Destroy();
	~Logger();
};

extern Logger* g_Logger;

#define LOG(fmt, ...) do { printf(fmt "\n", __VA_ARGS__); fprintf(g_Logger->file, fmt "\n", __VA_ARGS__); } while(0)

void LogInit(const char* filename);
void SetLogger(Logger* other);
