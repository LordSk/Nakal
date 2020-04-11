#pragma once
#include "base.h"

struct IpcMsgType
{
	enum Enum: i32
	{
		KEY_STROKE = 1
	};
};

struct IpcHeader
{
	int id;
};

struct IpcKeyStroke
{
	i32 vkey;
	i32 status; // 0 = pressed, 1 = released
};
