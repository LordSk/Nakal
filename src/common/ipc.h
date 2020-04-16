#pragma once
#include "base.h"

enum Enum: i32
{
	IPCMT_KEY_STROKE = 1,
	IPCMT_LOG_MSG = 2,
};

struct IpcHeader
{
	const i32 id;

	IpcHeader(int id_): id(id_) {}
};

struct IpcKeyStroke
{
	IpcHeader header = IpcHeader(IPCMT_KEY_STROKE);
	i32 vkey;
	i32 status; // 0 = pressed, 1 = released
};

struct IpcLogMsg
{
	IpcHeader header = IpcHeader(IPCMT_LOG_MSG);
	char text[256];
};
