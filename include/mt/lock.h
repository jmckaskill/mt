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
#include <mt/atomic.h>

/* Lock flags */
#define MT_LOCK_RECURSIVE 0x01

/* ------------------------------------------------------------------------- */

#if defined _WIN32

struct MT_Mutex {
    void*       DebugInfo;
    long        LockCount;
    long        RecursionCount;
    MT_Handle   OwningThread;
    MT_Handle   LockSemaphore;
    uint32_t    SpinCount;
};

struct _RTL_CRITICAL_SECTION;

MT_EXTERN_C
__declspec(dllimport)
void
__stdcall
EnterCriticalSection (
    struct _RTL_CRITICAL_SECTION* pcsCriticalSection
    );

MT_EXTERN_C
__declspec(dllimport)
void
__stdcall
LeaveCriticalSection (
    struct _RTL_CRITICAL_SECTION* pcsCriticalSection
    );

MT_EXTERN_C
__declspec(dllimport)
void
__stdcall
InitializeCriticalSection (
    struct _RTL_CRITICAL_SECTION* pcsCriticalSection
    );

MT_EXTERN_C
__declspec(dllimport)
void
__stdcall
DeleteCriticalSection (
    struct _RTL_CRITICAL_SECTION* pcsCriticalSection
    );

#define MT_MUTEX_INITIALIZER                                                \
    {                                                                       \
        NULL,       /* DebugInfo */                                         \
        -1,         /* LockCount */                                         \
        0,          /* RecursionCount */                                    \
        NULL,       /* OwningThread */                                      \
        NULL,       /* LockSemaphore */                                     \
        0           /* SpinCount */                                         \
    }

MT_INLINE void MT_InitMutex(MT_Mutex* lock)
{
    InitializeCriticalSection((struct _RTL_CRITICAL_SECTION*) lock);
}

MT_INLINE void MT_InitMutex2(MT_Mutex* lock, int flags)
{
    (void) flags;
    InitializeCriticalSection((struct _RTL_CRITICAL_SECTION*) lock);
}

MT_INLINE void MT_DestroyMutex(MT_Mutex* lock)
{
    DeleteCriticalSection((struct _RTL_CRITICAL_SECTION*) lock);
}

MT_INLINE void MT_Lock(MT_Mutex* lock)
{
    EnterCriticalSection((struct _RTL_CRITICAL_SECTION*) lock);
}

MT_INLINE void MT_Unlock(MT_Mutex* lock)
{
    LeaveCriticalSection((struct _RTL_CRITICAL_SECTION*) lock);
}

/* ------------------------------------------------------------------------- */

#else
#include <pthread.h>

struct MT_Mutex {
    pthread_mutex_t lock;
};

#define MT_MUTEX_INITIALIZER {PTHREAD_MUTEX_INITIALIZER}

MT_INLINE void MT_InitMutex(MT_Mutex* lock)
{
    pthread_mutex_init(&lock->lock, NULL);
}

MT_API void MT_InitMutex2(MT_Mutex* lock, int flags);

MT_INLINE void MT_DestroyMutex(MT_Mutex* lock)
{
    pthread_mutex_destroy(&lock->lock);
}

MT_INLINE void MT_Lock(MT_Mutex* lock)
{
    pthread_mutex_lock(&lock->lock);
}

MT_INLINE void MT_Unlock(MT_Mutex* lock)
{
    pthread_mutex_unlock(&lock->lock);
}

/* ------------------------------------------------------------------------- */

#endif

#ifdef __cplusplus
namespace MT
{

/* ------------------------------------------------------------------------- */

class Mutex
{
public:
    Mutex() {
        MT_InitMutex(&m_Lock);
    }

    Mutex(int flags) {
        MT_InitMutex2(&m_Lock, flags);
    }
    ~Mutex() {
        MT_DestroyMutex(&m_Lock);
    }

    void Lock() {
        MT_Lock(&m_Lock);
    }
    void Unlock() {
        MT_Unlock(&m_Lock);
    }

private:
    MT_Mutex m_Lock;
};

/* ------------------------------------------------------------------------- */

class ScopedLock
{
public:
    ScopedLock(MT::Mutex* lock) : m_Lock(lock) {
        m_Lock->Lock();
    }

    ~ScopedLock() {
        m_Lock->Unlock();
    }

private:
    MT::Mutex* m_Lock;
};

/* ------------------------------------------------------------------------- */

}
#endif

