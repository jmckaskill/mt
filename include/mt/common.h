/* vim: ts=4 sw=4 sts=4 et tw=78
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

#ifndef __STDC_LIMIT_MACROS
#   define __STDC_LIMIT_MACROS
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <delegate.h>

#ifdef __cplusplus
#define MT_EXTERN_C extern "C"
#else
#define MT_EXTERN_C extern
#endif

#if defined __cplusplus || __STDC_VERSION__ + 0 >= 199901L
#define MT_INLINE static inline
#else
#define MT_INLINE static
#endif

#if defined MT_STATIC_LIBRARY
#define MT_API MT_EXTERN_C

#elif defined _WIN32 && defined MT_LIBRARY
#define MT_API MT_EXTERN_C __declspec(dllexport)

#elif defined _WIN32 && !defined MT_LIBRARY
#define MT_API MT_EXTERN_C __declspec(dllimport)

#elif defined __GNUC__
#define MT_API MT_EXTERN_C __attribute__((visibility("default")))

#else
#define MT_API MT_EXTERN_C

#endif



#define MT_NOT_COPYABLE(c) private: c(c& __DummyArg); c& operator=(c& __DummyArg)

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
        ((type*) ((char*) ptr - offsetof(type, member)))
#endif
#endif


typedef struct MT_BufferedIO        MT_BufferedIO;
typedef struct MT_BrokenDownTime    MT_BrokenDownTime;
typedef struct MT_Directory         MT_Directory;
typedef struct MT_Event             MT_Event;
typedef struct MT_ExitMessage       MT_ExitMessage;
typedef struct MT_Http              MT_Http;
typedef struct MT_MessageQueue      MT_MessageQueue;
typedef struct MT_Mutex             MT_Mutex;
typedef struct MT_Process           MT_Process;
typedef struct MT_Publisher         MT_Publisher;
typedef struct MT_Reply             MT_Reply;
typedef struct MT_Request           MT_Request;
typedef struct MT_Sockaddr          MT_Sockaddr;
typedef struct MT_Object            MT_Object;
typedef struct MT_WeakData          MT_WeakData;
typedef struct MT_Thread            MT_Thread;
typedef struct MT_ThreadStorage     MT_ThreadStorage;

typedef struct MTI_DelegateVector   MTI_DelegateVector;

/* MT_Time gives the time in microseconds centered on the unix epoch (midnight
 * Jan 1 1970) */
typedef int64_t MT_Time;

#ifdef MT_NO_THREADS
typedef long MT_AtomicInt;
#else
typedef long volatile MT_AtomicInt;
#endif

typedef void (*MT_Callback)(void*);
typedef void* (*MT_CloneCallback)(void*);
typedef void (*MT_MessageCallback)(void*,const void*);

#ifdef _WIN32
typedef void* MT_Handle; /* HANDLE */
typedef uintptr_t MT_Socket; /* SOCKET */
#else
/* Handles on unix are file descriptors where we only care about read
 * being signalled. File descriptors that aren't actually files, pipes,
 * sockets etc normally fall in this category. This includes epoll
 * handles, thread wake up pipes (pipes that are just used to kick us out
 * out of poll), etc.
 */
typedef int MT_Handle;   /* fd_t */
typedef int MT_Socket;   /* fd_t */
#endif


