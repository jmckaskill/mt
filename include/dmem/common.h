/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * This software is licensed under the stock MIT license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#pragma once

#include <stddef.h>



#ifdef __cplusplus
#define DMEM_EXTERN_C extern "C"
#else
#define DMEM_EXTERN_C extern
#endif

#if defined __cplusplus || __STDC_VERSION__ + 0 >= 199901L
#define DMEM_INLINE static inline
#else
#define DMEM_INLINE static
#endif

#if defined DMEM_STATIC_LIBRARY
#define DMEM_API DMEM_EXTERN_C

#elif defined _WIN32 && defined DMEM_LIBRARY
#define DMEM_API DMEM_EXTERN_C __declspec(dllexport)

#elif defined _WIN32 && !defined DMEM_LIBRARY
#define DMEM_API DMEM_EXTERN_C __declspec(dllimport)

#elif defined __GNUC__
#define DMEM_API DMEM_EXTERN_C __attribute__((visibility("default")))

#else
#define DMEM_API DMEM_EXTERN_C

#endif


#ifndef NEW
#define NEW(type) (type*) calloc(1, sizeof(type))
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#ifdef __cplusplus
#define container_of(ptr, type, member) \
        ((type*) ((char*) ptr - (((char*) &((type*) 0x2000)->member) - 0x2000)))

#else
#define container_of(ptr, type, member) \
        ((type*) ((char*) (ptr) - offsetof(type, member)))
#endif
#endif


#ifndef STATIC_ASSERT
#define STATIC_ASSERT_CAT2_(x,y) x ## y
#define STATIC_ASSERT_CAT_(x,y) STATIC_ASSERT_CAT2_(x, y)
#define STATIC_ASSERT(x) struct STATIC_ASSERT_CAT_(Static_Assert_, __LINE__) {int Failed : (x) ? 1 : 0;}
#endif

/* Give the GCC attributes as this makes GCC verify the format string and the
 * arguments. FORMAT_ARG is the 1 based index of the format argument (add one
 * for the this argument of member functions). VARG_BEGIN is the first index of
 * the variable arguments (should be 0 for va_list args).
 */

#ifdef __GNUC__
#   define DMEM_PRINTF(FORMAT_ARG, VARG_BEGIN) __attribute__ ((format (printf, FORMAT_ARG, VARG_BEGIN)))
#else
#   define DMEM_PRINTF(FORMAT_ARG, VARG_BEGIN)
#endif

#ifdef __GNUC__
DMEM_API void* memmem(const void* hay, size_t haylen, const void* needle, size_t needlelen);
DMEM_API void* memchr(const void* s, int c, size_t n);
DMEM_API void* memrchr(const void* s, int c, size_t n);
#endif

DMEM_API void* memrmem(const void* hay, size_t haylen, const void* needle, size_t needlelen);

