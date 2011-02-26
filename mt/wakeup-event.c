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

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <limits.h>

#if (__GLIBC__ + 0) >= 2 && (__GLIBC_MINOR__ + 0) >= 8
#include <sys/eventfd.h>
#define MTI_USE_EVENTFD
#endif

#if (__GLIBC__ + 0) >= 2 && (__GLIBC_MINOR__ + 0) >= 9
#define MTI_USE_PIPE2
#endif

#endif


#include "wakeup-event.h"
#include "event-queue.h"


#ifdef _WIN32
#include <windows.h>

#else
#include <unistd.h>
#include <fcntl.h>
#define MTI_USE_PIPE
#define READ 0
#define WRITE 1

#endif

/* ------------------------------------------------------------------------- */

#ifdef MTI_USE_EVENTFD
static void MTI_WakeupEvent_OnUnixEvent(MTI_WakeupEvent* s)
{
    eventfd_t val;
    eventfd_read(s->unix_event, &val);
    CALL_DELEGATE_0(s->call);
}
#endif

/* ------------------------------------------------------------------------- */

#if defined MTI_USE_PIPE || defined MTI_USE_PIPE2
static void MTI_WakeupEvent_OnPipe(MTI_WakeupEvent* s)
{
    char buf[256];
    read(s->unix_pipe[READ], buf, 256);
    CALL_DELEGATE_0(s->call);
}
#endif

/* ------------------------------------------------------------------------- */

void MTI_InitWakeupEvent(MTI_WakeupEvent* s, MTI_EventQueue* q, VoidDelegate cb)
{
    /* Default the handles */
#ifdef _WIN32
    s->win_event = INVALID_HANDLE_VALUE;
#else
    s->unix_pipe[READ] = -1;
    s->unix_pipe[WRITE] = -1;
    s->unix_event = -1;
#endif

    s->call = cb;

    /* Try each handle mechanism in turn */
#ifdef _WIN32
    s->win_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (s->win_event != INVALID_HANDLE_VALUE) {
        s->event = MTI_NewHandleEvent(q, s->win_event, cb);
        return;
    }
#endif

#ifdef MTI_USE_EVENTFD
    s->unix_event = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (s->unix_event >= 0) {
        s->event = MTI_NewHandleEvent(q, s->unix_event, BindVoid(&MTI_WakeupEvent_OnUnixEvent, s));
        return;
    }
#endif

#ifdef MTI_USE_PIPE2
    if (!pipe2(s->unix_pipe, O_CLOEXEC | O_NONBLOCK)) {
        s->event = MTI_NewHandleEvent(q, s->unix_pipe[READ], BindVoid(&MTI_WakeupEvent_OnPipe, s));
        return;
    }
#endif

#ifdef MTI_USE_PIPE
    if (pipe(s->unix_pipe)) {
        fcntl(s->unix_pipe[READ], F_SETFD, FD_CLOEXEC);
        fcntl(s->unix_pipe[WRITE], F_SETFD, FD_CLOEXEC);
        fcntl(s->unix_pipe[READ], F_SETFL, O_NONBLOCK);
        fcntl(s->unix_pipe[WRITE], F_SETFL, O_NONBLOCK);

        s->event = MTI_NewHandleEvent(q, s->unix_pipe[READ], BindVoid(&MTI_WakeupEvent_OnPipe, s));
        return;
    }
#endif
}

/* ------------------------------------------------------------------------- */

void MTI_DestroyWakeupEvent(MTI_WakeupEvent* s)
{
    MT_FreeEvent(s->event);

#ifdef _WIN32
    if (s->win_event != INVALID_HANDLE_VALUE) {
        CloseHandle(s->win_event);
    }
#else
    if (s->unix_pipe[READ] >= 0) {
        close(s->unix_pipe[READ]);
    }

    if (s->unix_pipe[WRITE] >= 0) {
        close(s->unix_pipe[WRITE]);
    }

    if (s->unix_event >= 0) {
        close(s->unix_event);
    }
#endif
}

/* ------------------------------------------------------------------------- */

void MTI_TriggerWakeupEvent(MTI_WakeupEvent* s)
{
#ifdef _WIN32
    SetEvent(s->win_event);
#else
    if (s->unix_event >= 0) {
        eventfd_write(s->unix_event, 1);

    } else if (s->unix_pipe[WRITE] >= 0) {
        char ch = 0;
        write(s->unix_pipe[WRITE], &ch, 1);

    }
#endif
}
