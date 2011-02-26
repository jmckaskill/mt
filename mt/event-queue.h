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

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif

#include "mt-internal.h"
#include "wakeup-event.h"
#include <mt/message.h>
#include <mt/event.h>
#include <dmem/vector.h>
#include <stdbool.h>

#ifdef _WIN32
typedef struct {
    long events;
    long revents;
} MTI_Event;

#else
typedef struct pollfd MTI_Event;
#define FD_READ     POLLIN
#define FD_WRITE    POLLOUT
#define FD_CLOSE    POLLHUP
#define FD_ACCEPT   POLLIN

#endif

/* ------------------------------------------------------------------------- */

enum MTI_RegistrationType {
    MTI_CLIENT_SOCKET,
    MTI_SERVER_SOCKET,
    MTI_TICK,
    MTI_IDLE

#ifdef _WIN32
    , MTI_HANDLE
#endif
};

/* ------------------------------------------------------------------------- */

struct MT_Event {
    enum MTI_RegistrationType       type;
    MTI_EventQueue*                 event_queue;

    MT_Socket                       socket;

    MT_Time                         period;
    MT_Time                         next_tick;

    bool                            enabled;

    VoidDelegate                    on_read;
    VoidDelegate                    on_write;
    VoidDelegate                    on_close;
    VoidDelegate                    on_accept;
    VoidDelegate                    on_idle;
    VoidDelegate                    on_tick;

#ifdef _WIN32
    MT_Handle                       handle;
    VoidDelegate                    on_handle;
#endif
};

/* ------------------------------------------------------------------------- */

DVECTOR_INIT(EventRegistration, MT_Event*);
DVECTOR_INIT(Event, MTI_Event);
DVECTOR_INIT(Handle, MT_Handle);

struct MTI_EventQueue {
    bool                        exit;

    d_Vector(EventRegistration) socket_regs;

    /* Kept sorted by next_tick field */
    d_Vector(EventRegistration) tick_regs;

    d_Vector(EventRegistration) idle_regs;
    int                         next_idle;

    /* List of already known events */
    d_Vector(Event)             events;
    int                         next_event;

#ifdef _WIN32
    /* Handles are redirected to a socket on unix */
    d_Vector(EventRegistration) handle_regs;
    /* Has all of the socket_regs handles followed by the handle_regs */
    d_Vector(Handle)            handles;
#endif

    MTI_WakeupEvent             wakeup;
};

MTI_EventQueue* MTI_CurrentEventQueue(void);

void MTI_InitEventQueue(MTI_EventQueue* s, MT_MessageQueue* q);
void MTI_DestroyEventQueue(MTI_EventQueue* s);
void MTI_ExitEventQueue(MTI_EventQueue* s);

MT_Event* MTI_NewHandleEvent(MTI_EventQueue* s, MT_Handle h, VoidDelegate cb);

