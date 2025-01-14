#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

#include "error.h"

#define VERSION "0.1.0"

#define len(array) (sizeof(array) / sizeof((array)[0]))

#define UNUSED(var) (void)(var)

#ifdef INFINITY
	#define INF INFINITY
#else
	#define INF (1.0 / 0.0)
#endif

#define array(type) struct { usize len; usize cap; type *buf; }
#define make(type, size) { .len = 0, .cap = (size), .buf = malloc(sizeof(type) * (size)) }
#define init(var, size) \
    { \
        (var).len = 0; \
        (var).cap = size; \
        (var).buf = malloc(sizeof(*(var).buf) * size); \
    }
#define grow(type, arr) \
	if ((arr)->len >= (arr)->cap) { \
		usize newsize = (arr)->len * 2 + 1; \
		(arr)->cap = newsize; \
		newsize *= sizeof(type); \
		(arr)->buf = realloc((arr)->buf, newsize); \
	}

extern const bool debug;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef size_t usize;
typedef ptrdiff_t ssize;

#if (CHAR_BIT == 8)
	#if (CHAR_MIN < 0)
		typedef unsigned char byte;
	#else
		typedef char byte;
	#endif
#else
	typedef u8 byte;
#endif

typedef float f32;
typedef double f64;
typedef long double fsize;

ssize ipow(ssize, usize);

byte *remove_all_bytes(const byte *, byte);
byte *trim(const byte *);
byte *downcase(const byte *);

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Check windows
#if defined(_WIN32) || defined(_WIN64)
	#ifdef _WIN64
		#define ARCH64
	#else
		#define ARCH32
	#endif
#endif

// Check GCC
#ifdef __GNUC__
	#if defined(__x86_64__) || defined(__ppc64__)
		#define ARCH64
	#else
		#define ARCH32
	#endif
#endif

#if defined(_MSC_VER)
	#define COMPILER "Visual Studio " STR(VS)
#elif defined(__GNUC__)
	#define COMPILER "GCC " STR(__GNUC__) "." STR(__GNUC_MINOR__)
#elif defined(__clang__)
	#define COMPILER "Clang " STR(__clang_major__) "." STR(__clang_minor__)
#elif defined(__EMSCRIPTEN__)
	#define COMPILER "WebAssembly " STR(__EMSCRIPTEN__)
#elif defined(__MINGW32__)
	#define COMPILER "MinGW 32bit " STR(__MINGW32__)
#elif defined(__MINGW64__)
	#define COMPILER "MinGW 64bit " STR(__MINGW64__)
#else
	#define COMPILER "Unknown Compiler"
#endif
