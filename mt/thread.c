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

/* To get pthread_mutexattr_settype */
#define _XOPEN_SOURCE 500

#include "thread.h"
#include "message-queue.h"
#include <mt/lock.h>
#include <dmem/vector.h>
#include <assert.h>

#if defined _WIN32 && !defined _WIN32_WCE
/* For _beginthreadex */
#include <process.h>
#endif

#if defined _WIN32
#include <windows.h>
#endif

static void JoinThread(MT_Thread* s);

/* ------------------------------------------------------------------------- */

MT_Thread* MT_NewThread2(const char* name, va_list ap)
{
    MT_Thread* s = NEW(MT_Thread);
    MT_InitSignal(int, &s->on_exit);
    dv_vprint(&s->name, name, ap);
    s->message_queue = MT_NewMessageQueue();
    return s;
}

MT_Thread* MT_NewThread(const char* name, ...)
{
    va_list ap;
    va_start(ap, name);
    return MT_NewThread2(name, ap);
}

/* ------------------------------------------------------------------------- */

void MT_FreeThread(MT_Thread* s)
{
    if (s) {
        MT_LOG("Thread free %p", s);

        if (s->old_message_queue) {
            MT_EndThreadInit(s);
        }

        if (!s->joined) {
            MT_ExitThread(s);
            JoinThread(s);
        }

        MT_FreeMessageQueue(s->message_queue);
        MT_DestroySignal(&s->on_exit);
        dv_free(s->name);
        free(s);
    }
}

/* ------------------------------------------------------------------------- */

void MT_BeginThreadInit(MT_Thread* s)
{
    assert(!s->started);
    s->old_message_queue = MT_CurrentMessageQueue();
    MT_SetCurrentMessageQueue(s->message_queue);
}

/* ------------------------------------------------------------------------- */

void MT_EndThreadInit(MT_Thread* s)
{
    assert(!s->started);
    assert(MT_CurrentMessageQueue() == s->message_queue);
    MT_SetCurrentMessageQueue(s->old_message_queue);
    s->old_message_queue = NULL;
}

/* ------------------------------------------------------------------------- */

static MT_ThreadStorage g_thread_name = MT_THREAD_STORAGE_INITIALIZER;

const char* MT_GetCurrentThreadName(void)
{
    return (const char*) MT_GetThreadStorage(&g_thread_name);
}

static void SetCurrentThreadName(const char* name)
{
#if defined _WIN32 && !defined _WIN32_WCE && defined DEBUG && defined _MSC_VER

#pragma warning(disable:4133) /* RaiseException prototype keeps on changing types */

    struct {
        DWORD dwType;      /* must be 0x1000 */
        LPCSTR szName;     /* pointer to name (in user addr space) */
        DWORD dwThreadID;  /* thread ID (-1=caller thread) */
        DWORD dwFlags;     /* reserved for future use, must be zero */
    } info;

    info.dwType     = 0x1000;
    info.szName     = name;
    info.dwThreadID = (DWORD) -1;
    info.dwFlags    = 0;

    __try {
        RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), &info);
    } __except(EXCEPTION_CONTINUE_EXECUTION) {
    }
#endif

    MT_SetThreadStorage(&g_thread_name, (void*) name);
}

/* ------------------------------------------------------------------------- */

#ifdef _WIN32
static unsigned int __stdcall
#else
static void* 
#endif
ThreadStart(void* arg)
{
    MT_Thread* s = (MT_Thread*) arg;
    int exit_code;

    if (s->name.size) {
        SetCurrentThreadName(s->name.data);
    }

    MT_SetCurrentMessageQueue(s->message_queue);
    exit_code = CALL_DELEGATE_0(s->start);

    MT_Emit(&s->on_exit, &exit_code);

#ifdef _WIN32
    return (DWORD) exit_code;
#else
    return (void*) (intptr_t) exit_code;
#endif
}

/* ------------------------------------------------------------------------- */

MT_Signal(int)* MT_ThreadOnExit(MT_Thread* s)
{
    return &s->on_exit;
}

/* ------------------------------------------------------------------------- */

void MT_ExitThread(MT_Thread* s)
{
    MTI_ExitEventQueue(&s->message_queue->event_queue);
}

/* ------------------------------------------------------------------------- */

void MT_StartThread(MT_Thread* s, IntDelegate call)
{
    assert(!s->started);

    MT_LOG("Thread start %p", s);

    if (s->old_message_queue) {
        MT_EndThreadInit(s);
    }

    s->start = call;
    s->started = true;

#if defined _WIN32_WCE
    s->thread = CreateThread(NULL, 0, &ThreadStart, s, 0, NULL);
#elif defined _WIN32
    s->thread = (MT_Handle) _beginthreadex(NULL, 0, &ThreadStart, s, 0, NULL);
#else
    pthread_create(&s->thread, NULL, &ThreadStart, s);
#endif
}

/* ------------------------------------------------------------------------- */

static void JoinThread(MT_Thread* s)
{
    MT_LOG("Thread join %p", s);

    if (s->started && !s->joined) {
#   ifdef _WIN32
        WaitForSingleObject(s->thread, INFINITE);
        CloseHandle(s->thread);
        s->thread = INVALID_HANDLE_VALUE;
#   else
        pthread_join(s->thread, NULL);
#   endif
    }

    s->joined = true;
}





/* ------------------------------------------------------------------------- */

static MT_Mutex g_tls_create_lock = MT_MUTEX_INITIALIZER;

void MT_SetThreadStorage(MT_ThreadStorage* s, void* val)
{
    void* old = MT_GetThreadStorage(s);

    /* Increment the ref and create the key if needed */
    if (val && !old) {
        MT_Lock(&g_tls_create_lock);

        if (s->ref == 0) {
#if defined _WIN32
            s->tls = TlsAlloc();
#else
            pthread_key_create(&s->tls, NULL);
#endif
        }

        MT_AtomicIncrement(&s->ref);
        MT_Unlock(&g_tls_create_lock);
    }

    /* Set the tls value */
    if (s->ref > 0) {
#if defined _WIN32
        TlsSetValue(s->tls, val);
#else
        pthread_setspecific(s->tls, val);
#endif
    }

    /* Decrement the ref and destroy the key if needed */
    if (!val && old) {
        MT_Lock(&g_tls_create_lock);

        if (MT_AtomicDecrement(&s->ref) == 0) {
#if defined _WIN32
            TlsFree(s->tls);
#else
            pthread_key_delete(s->tls);
#endif
        }

        MT_Unlock(&g_tls_create_lock);
    }
}

/* ------------------------------------------------------------------------- */

#if !defined _WIN32
void MT_InitMutex2(MT_Mutex* lock, int flags)
{
    if (flags & MT_LOCK_RECURSIVE) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&lock->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&lock->lock, NULL);
    }
}
#endif

