#pragma once

#include <atomic>

typedef	uint8_t							u8;
typedef	int8_t							s8;
typedef uint16_t						u16;
typedef int16_t							s16;
typedef uint32_t						u32;
typedef int32_t							s32;
typedef uint64_t						u64;
typedef int64_t							s64;
typedef float							f32;
typedef double							f64;
typedef intptr_t						intptr;	//	really just to remind myself it exists

typedef	std::atomic<bool>				abool;
typedef	std::atomic<u8>					au8;
typedef	std::atomic<s8>					as8;
typedef std::atomic<u16>				au16;
typedef std::atomic<s16>				as16;
typedef std::atomic<u32>				au32;
typedef std::atomic<s32>				as32;
typedef std::atomic<u64>				au64;
typedef std::atomic<s64>				as64;
typedef std::atomic<f32>				af32;
typedef std::atomic<f64>				af64;

typedef f32								coord_type;
typedef f32								rot_type;