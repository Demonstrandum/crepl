#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "error.h"

#define len(array) (sizeof(array) / sizeof((array)[0]))

#ifdef INFINITY
	#define INF INFINITY
#else
	#define INF (1.0 / 0.0)
#endif

typedef uint8_t u8 ;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8 ;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef size_t usize;
typedef ptrdiff_t ssize;

typedef float f32;
typedef double f64;
typedef long double fsize;

ssize ipow(ssize, usize);

char *remove_all_char(const char *, char);
char *trim(const char *);
char *downcase(const char *);

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

