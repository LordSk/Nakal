#pragma once
#include <stdio.h>
#include <stdint.h>

#define LOG(fmt, ...) printf(fmt "\n", __VA_ARGS__)

typedef uint32_t u32;
typedef int32_t i32;
typedef int64_t i64;

#define ASSERT(cond) if(!(cond)) { __debugbreak(); }

#define ARRAY_COUNT(A) (sizeof(A)/sizeof(A[0]))
