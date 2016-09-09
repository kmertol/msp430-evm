/* Copyright (c) 2016 Kaan Mertol
 * Licensed under the MIT License. See the accompanying LICENSE file */

#ifndef TYPES_H_
#define TYPES_H_

#include <stdint.h>

#define	NULL   ((void*)0)
#define	Null   ((void*)0)
#define	null   ((void*)0)
#define TRUE   1
#define True   1
#define FALSE  0
#define False  0

#ifndef _STDBOOL
#define _STDBOOL
	typedef uint8_t bool;
	#define false	0
	#define true	1
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint_fast8_t u8f;
typedef uint_fast16_t u16f;
typedef uint_fast32_t u32f;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef int_fast8_t s8f;
typedef int_fast16_t s16f;
typedef int_fast32_t s32f;
typedef unsigned int uint;

#ifndef _SIZE_T
#define _SIZE_T

#ifndef __SIZE_T_TYPE__
typedef unsigned int size_t;
#else
typedef __SIZE_T_TYPE__ size_t;
#endif

#endif

typedef void (*pfn_t)(void);

/****************************************************************************/
// get offsetof a member in a struct
#define offsetof(_type, _ident) ((size_t)__intaddr__(&(((_type *)0)->_ident)))
// array OR item size, will give error if uneven spacing
#define countof(x)	((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#endif
