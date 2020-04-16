#include "logger.h"

static Logger logger;
Logger* g_Logger = &logger;

void Logger::Init(const char* filename)
{
	if(file == nullptr) {
		file = fopen(filename, "wb");
		ASSERT(file);
	}
}

void Logger::Destroy()
{
	if(file) {
		fclose(file);
	}
}

Logger::~Logger()
{
	Destroy();
}

void LogInit(const char* filename)
{
	logger.Init(filename);
}

void SetLogger(Logger* other)
{
	g_Logger = other;
}
