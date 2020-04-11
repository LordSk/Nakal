#pragma once
#include <stdio.h>
#include <stdint.h>

typedef uint32_t u32;
typedef int32_t i32;
typedef int64_t i64;

#define ASSERT(cond) do { if(!(cond)) { __debugbreak(); } } while(0)
#ifdef CONF_DEBUG
	#define DBG_ASSERT(cond) ASSERT(cond)
#else
	#define DBG_ASSERT(cond)
#endif

#define ARRAY_COUNT(A) (sizeof(A)/sizeof(A[0]))

#define NAKAL_WND_CLASS L"Nakal_1564568463"

template<typename T, u32 CAPACITY>
struct PlainArray
{
	enum {
		CAPACITY = CAPACITY,
	};

	T data[CAPACITY];
	i32 count = 0;

	inline T& Push(T elt)
	{
		ASSERT(count < CAPACITY);
		return data[count++] = elt;
	}

	inline T& PushMany(const T* list, int listCount)
	{
		ASSERT(count + listCount <= CAPACITY);
		memmove(data + count, list, sizeof(T) * listCount);
		count += listCount;
		return data[count-listCount];
	}

	void Remove(const T& elt)
	{
		const i32 ID = &elt - data;
		RemoveByID(ID);
	}

	void RemoveByID(u32 ID)
	{
		ASSERT(ID >= 0 && ID < count);
		memmove(&data[ID], &data[count-1], sizeof(T));
		count--;
	}

	void RemoveByIDKeepOrder(u32 ID)
	{
		ASSERT(ID >= 0 && ID < count);
		memmove(&data[ID], &data[ID+1], sizeof(T) * (count-ID-1));
		count--;
	}

	inline void Clear()
	{
		count = 0;
	}

	inline i32 Count() const
	{
		return count;
	}

	inline bool IsFull() const
	{
		return count >= CAPACITY;
	}

	inline T* Data()
	{
		return data;
	}

	inline const T* Data() const
	{
		return data;
	}

	inline T& operator [] (u32 index)
	{
		DBG_ASSERT((i32)index < count);
		return data[index];
	}

	inline const T& operator [] (u32 index) const
	{
		DBG_ASSERT((i32)index < count);
		return data[index];
	}
};
