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

#include <mt/common.h>
#include <mt/message.h>
#include <stdarg.h>

#if defined _WIN32
typedef unsigned long MT_ThreadStorageKey; /* DWORD */
#else
#include <pthread.h>
typedef pthread_key_t MT_ThreadStorageKey;
#endif

/* ------------------------------------------------------------------------- */

MT_API MT_Thread* MT_NewThread(const char* name, ...);
MT_API MT_Thread* MT_NewThread2(const char* name, va_list ap);
MT_API void MT_FreeThread(MT_Thread* thread);

/* After calling BeginInit and until EndInit or Start is called the local
 * state is setup as if its on the new thread. Thus its a good time to
 * initialise any objects that are to be used on the new thread.
 */
MT_API void MT_BeginThreadInit(MT_Thread* s);
MT_API void MT_EndThreadInit(MT_Thread* s);

MT_API MT_Signal(int)* MT_ThreadOnExit(MT_Thread* s);
MT_API void MT_StartThread(MT_Thread* s, IntDelegate call);
MT_API void MT_ExitThread(MT_Thread* s);
MT_API void MT_SetThreadName(MT_Thread* s, const char* name);

/* ------------------------------------------------------------------------- */

MT_API const char* MT_GetCurrentThreadName(void);

MT_API void MT_ExitEventLoop(void);
MT_API void MT_RunEventLoop(void);
MT_API void MT_StepEventLoop(void);

/* ------------------------------------------------------------------------- */

#define MT_THREAD_STORAGE_INITIALIZER {0, 0}

struct MT_ThreadStorage {
    MT_ThreadStorageKey     tls;
    MT_AtomicInt            ref;
};

MT_API void MT_SetThreadStorage(MT_ThreadStorage* s, void* val);

#if defined _WIN32
__declspec(dllimport) void* __stdcall TlsGetValue(unsigned long dwTlsIndex);

MT_INLINE void* MT_GetThreadStorage(MT_ThreadStorage* s)
{ return s->ref > 0 ? TlsGetValue(s->tls) : NULL; }

#else
MT_INLINE void* MT_GetThreadStorage(MT_ThreadStorage* s)
{ return s->ref > 0 ? pthread_getspecific(s->tls) : NULL; }

#endif





