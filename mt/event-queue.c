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


#include "event-queue.h"
#include "message-queue.h"
#include <mt/thread.h>
#include <mt/time.h>
#include <limits.h>
#include <assert.h>

static void StepEventQueue(MTI_EventQueue* s);

/* ------------------------------------------------------------------------- */

static MTI_EventQueue* CreateCurrentEventQueue(void)
{
    MT_MessageQueue* q = MTI_CreateCurrentMessageQueue();
    return &q->event_queue;
}

/* ------------------------------------------------------------------------- */

void MTI_InitEventQueue(MTI_EventQueue* s, MT_MessageQueue* q)
{
    memset(s, 0, sizeof(MTI_EventQueue));

    s->exit = false;
    s->next_idle = 0;
    s->next_event = -1;

    MTI_InitWakeupEvent(&s->wakeup, s, BindVoid(&MT_ProcessMessageQueue, q));
    MT_SetMessageQueueWakeup(q, BindVoid(&MTI_TriggerWakeupEvent, &s->wakeup));
}

/* ------------------------------------------------------------------------- */

void MTI_DestroyEventQueue(MTI_EventQueue* s)
{
    MTI_DestroyWakeupEvent(&s->wakeup);

    assert(s->events.size == 0);
    dv_free(s->events);

    assert(s->socket_regs.size == 0);
    assert(s->idle_regs.size == 0);
    assert(s->tick_regs.size == 0);

    dv_free(s->socket_regs);
    dv_free(s->idle_regs);
    dv_free(s->tick_regs);

#ifdef _WIN32
    assert(s->handle_regs.size == 0);
    assert(s->handles.size == 0);
    dv_free(s->handle_regs);
    dv_free(s->handles);
#endif
}

/* ------------------------------------------------------------------------- */
 
void MTI_ExitEventQueue(MTI_EventQueue* s)
{
    s->exit = true;
    MTI_TriggerWakeupEvent(&s->wakeup);
}

void MT_ExitEventLoop(void)
{
    MTI_EventQueue* s = CreateCurrentEventQueue();
    s->exit = true;
}

/* ------------------------------------------------------------------------- */

void MT_RunEventLoop(void)
{
    MTI_EventQueue* s = CreateCurrentEventQueue();

    while (!s->exit) {
        StepEventQueue(s);
    }
}

/* ------------------------------------------------------------------------- */

static int FindFirstGreaterOrEqual(d_Vector(EventRegistration)* vec, MT_Time nextTick)
{
    int sz = vec->size;
    int half;
    int begin, middle, end;

    begin = 0;
    end   = sz;

    /* This is a modified version of std::upper_bound where the comparison
     * operator has been changed from '<' to '<='.
     */
    while (sz > 0) {
        half = sz / 2;
        middle = begin + half;

        if (nextTick <= vec->data[middle]->next_tick) {
            /* upper bound is in first half */
            sz = half;
        } else {
            /* upper bound is in second half */
            begin = middle + 1;
            sz = sz - half - 1;
        }
    }

    return begin;
}

/* ------------------------------------------------------------------------- */

static MT_Event* NewSocketEvent(
    MTI_EventQueue*     s,
    MT_Socket           sock,
    VoidDelegate         read,
    VoidDelegate         write,
    VoidDelegate         close,
    VoidDelegate         accept)
{
    MT_Event* r;
    MTI_Event* e;

    assert(read.func || write.func || close.func || accept.func);

#ifdef _WIN32
    if (sock == INVALID_SOCKET) {
        return NULL;
    }
#else
    if (sock < 0) {
        return NULL;
    }
#endif

    r               = NEW(MT_Event);
    r->event_queue  = s;
    r->socket       = sock;
    r->on_read      = read;
    r->on_write     = write;
    r->on_close     = close;
    r->on_accept    = accept;
    r->enabled      = true;

    e = dv_append_zeroed(&s->events, 1);

    if (read.func) {
        e->events |= FD_READ;
    }

    if (write.func) {
        e->events |= FD_WRITE;
    }

    if (close.func) {
        e->events |= FD_CLOSE;
    }

    if (accept.func) {
        e->events |= FD_ACCEPT;
    }

    {
#ifdef _WIN32
        r->handle = WSACreateEvent();
        dv_insert2(&s->handles, s->socket_regs.size, &r->handle, 1);
        WSAEventSelect(sock, r->handle, e->events);
#else
        int flags = fcntl(sock, F_GETFL);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        e->fd = sock;
#endif
    }

    dv_append2(&s->socket_regs, &r, 1);

    return r;
}

/* ------------------------------------------------------------------------- */

MT_Event* MT_NewClientSocketEvent(MT_Socket sock, VoidDelegate read, VoidDelegate write, VoidDelegate close)
{
    VoidDelegate null = NULL_DELEGATE;
    MT_Event* r = NewSocketEvent(CreateCurrentEventQueue(), sock, read, write, close, null);

    if (r) {
        r->type = MTI_CLIENT_SOCKET;
    }

    return r;
}

/* ------------------------------------------------------------------------- */

MT_Event* MT_NewServerSocketEvent(MT_Socket sock, VoidDelegate accept)
{
    VoidDelegate null = NULL_DELEGATE;
    MT_Event* r = NewSocketEvent(CreateCurrentEventQueue(), sock, null, null, null, accept);

    if (r) {
        r->type = MTI_SERVER_SOCKET;
    }

    return r;
}

/* ------------------------------------------------------------------------- */

#ifdef _WIN32
MT_Event* MTI_NewHandleEvent(MTI_EventQueue* s, MT_Handle h, VoidDelegate cb)
{
    MT_Event* r;

    assert(s && cb.func);

    if (h == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    r               = NEW(MT_Event);
    r->event_queue  = s;
    r->type         = MTI_HANDLE;
    r->handle       = h;
    r->on_handle    = cb;

    MT_EnableEvent(r, MT_EVENT_HANDLE);

    return r;
}

#else
MT_Event* MTI_NewHandleEvent(MTI_EventQueue* s, MT_Handle h, VoidDelegate cb)
{
    VoidDelegate null = NULL_DELEGATE;
    MT_Event* r = NewSocketEvent(s, h, cb, null, null, null);

    if (r) {
        r->type = MTI_CLIENT_SOCKET;
    }

    return r;
}
#endif

MT_Event* MT_NewHandleEvent(MT_Handle h, VoidDelegate cb)
{ return MTI_NewHandleEvent(CreateCurrentEventQueue(), h, cb); }

/* ------------------------------------------------------------------------- */

MT_Event* MT_NewTickEvent(MT_Time period, VoidDelegate cb)
{
    MT_Event* r;

    assert(cb.func && period > 0);

    r               = NEW(MT_Event);
    r->event_queue  = CreateCurrentEventQueue();
    r->type         = MTI_TICK;
    r->period       = period;
    r->on_tick      = cb;

    MT_EnableEvent(r, MT_EVENT_TICK);

    return r;
}

/* ------------------------------------------------------------------------- */

MT_Event* MT_NewIdleEvent(VoidDelegate cb)
{
    MT_Event* r;

    assert(cb.func);

    r               = NEW(MT_Event);
    r->event_queue  = CreateCurrentEventQueue();
    r->type         = MTI_IDLE;
    r->on_idle      = cb;

    MT_EnableEvent(r, MT_EVENT_IDLE);

    return r;
}

/* ------------------------------------------------------------------------- */

static int HandleEvent(MTI_EventQueue* s)
{
    /* Win32 will only ever have one pending event at a time */
#ifdef _WIN32
    if (0 <= s->next_event && s->next_event < s->events.size)
#else
    while (0 <= s->next_event && s->next_event < s->events.size)
#endif
    {
        MTI_Event* e = &s->events.data[s->next_event];
        MT_Event* r = s->socket_regs.data[s->next_event];

        if (r->type == MTI_CLIENT_SOCKET) {
            if ((e->revents & FD_READ) && (e->events & FD_READ) && r->on_read.func) {
                e->revents &= ~FD_READ;
                CALL_DELEGATE_0(r->on_read);
                return 1;
            }

            if ((e->revents & FD_CLOSE) && (e->events & FD_CLOSE) && r->on_close.func) {
                e->revents &= ~FD_CLOSE;
                CALL_DELEGATE_0(r->on_close);
                return 1;
            }

            if ((e->revents & FD_WRITE) && (e->events & FD_WRITE) && r->on_write.func) {
                e->revents &= ~FD_WRITE;
                CALL_DELEGATE_0(r->on_write);
                return 1;
            }

        } else {
            assert(r->type == MTI_SERVER_SOCKET);
            assert((e->revents & FD_CLOSE) == 0);

            if ((e->revents & FD_ACCEPT) && (e->events & FD_ACCEPT) && r->on_accept.func) {
                e->revents &= ~FD_ACCEPT;
                CALL_DELEGATE_0(r->on_accept);
                return 1;
            }
        }

#ifdef _WIN32
        s->next_event = -1;
#else
        s->next_event++;
#endif
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

#ifdef _WIN32
static int GetNewEvents(MTI_EventQueue* s, MT_Time timeout)
{
    DWORD ret;
    DWORD time = INFINITE;

    if (MT_TIME_ISVALID(timeout)) {
        time = (DWORD) MT_TIME_TO_MS(timeout);
    }

    ret = WaitForMultipleObjects(
              (DWORD) s->handles.size,
              s->handles.data,
              FALSE,                  /* Wait for all */
              time);

    if (0 <= ret && ret < (DWORD) s->socket_regs.size) {
        /* A socket was signalled */
        WSANETWORKEVENTS events;
        MT_Event* r = s->socket_regs.data[ret];
        MTI_Event* e = &s->events.data[ret];

        if (WSAEnumNetworkEvents(r->socket, r->handle, &events)) {
            return 1;
        }

        e->revents = events.lNetworkEvents;
        s->next_event = ret;
        HandleEvent(s);
        return 1;

    } else if (ret < (DWORD) s->handles.size) {
        /* A handle was signalled */
        MT_Event* r = s->handle_regs.data[ret - s->socket_regs.size];
        CALL_DELEGATE_0(r->on_handle);
        return 1;

    } else if (ret == WAIT_TIMEOUT) {
        return 0;

    } else {
        /* error */
        return 1;
    }
}

#else
static int GetNewEvents(MTI_EventQueue* s, MT_Time timeout)
{
    int ret;
    int timeoutms = -1;

    if (MT_TIME_ISVALID(timeout)) {
        timeoutms = MT_TIME_TO_MS(timeout);
    }

    ret = poll(s->events.data, s->events.size, timeoutms);

    if (ret < 0) {
        /* error */
        return 1;

    } else if (ret == 0) {
        /* timeout */
        return 0;

    } else {
        /* One or more events have been returned */
        s->next_event = 0;
        HandleEvent(s);
        return 1;
    }
}

#endif

/* ------------------------------------------------------------------------- */

static void StepEventQueue(MTI_EventQueue* s)
{
    MT_Time current_time = MT_TIME_INVALID;
    assert(s->events.size == s->socket_regs.size);

    /* 1. Handle already known events */
    if (s->next_event >= 0 && HandleEvent(s)) {
        return;
    }

    s->next_event = -1;

    /* 2. Handle expired timeout */
    if (s->tick_regs.size > 0) {
        MT_Event* r = s->tick_regs.data[0];
        current_time = MT_CurrentTime();

        if (current_time >= r->next_tick) {
            /* New insert position is the index in the range [1,num of regs)
             * where we want to move the just expired reg to.
             */
            int insert_pos;

            dv_erase(&s->tick_regs, 0, 1);

            r->next_tick += r->period;
            insert_pos = FindFirstGreaterOrEqual(&s->tick_regs, r->next_tick);
            dv_insert2(&s->tick_regs, insert_pos, &r, 1);

            CALL_DELEGATE_0(r->on_tick);
            return;
        }
    }

    if (s->idle_regs.size > 0) {
        /* 3. Get OS events with a 0 timeout */
        if (GetNewEvents(s, 0)) {
            return;
        }

        /* 4. Handle idle */
        if (s->next_idle < s->idle_regs.size) {
            MT_Event* r = s->idle_regs.data[s->next_idle];
            s->next_idle++;
            CALL_DELEGATE_0(r->on_idle);
            return;
        }

        s->next_idle = 0;
    }

    {
        /* 5. Get OS events with a timeout and block */
        MT_Time timeout = MT_TIME_INVALID;

        if (s->tick_regs.size > 0) {
            timeout = s->tick_regs.data[0]->next_tick - current_time;
            assert(MT_TIME_ISVALID(current_time));
            assert(MT_TIME_ISVALID(timeout) && timeout > 0);
        }

        if (GetNewEvents(s, timeout)) {
            return;
        }
    }

    {
        /* 6. Handle expired timeout. No need to get current_time and check
         * if the timeout has actually expired as the OS event block in #5
         * should guarantee that the timeout has expired.
         */
        MT_Event* r = s->tick_regs.data[0];

        /* New insert position is the index in the range [1,num of regs) where
         * we want to move the just expired reg to.
         */
        int insert_pos;
        
        dv_erase(&s->tick_regs, 0, 1);

        r->next_tick += r->period;
        insert_pos = FindFirstGreaterOrEqual(&s->tick_regs, r->next_tick);
        dv_insert2(&s->tick_regs, insert_pos, &r, 1);

        CALL_DELEGATE_0(r->on_tick);
        return;
    }
}

void MT_StepEventLoop(void)
{
    MTI_EventQueue* eq = CreateCurrentEventQueue();
    StepEventQueue(eq);
}

/* ------------------------------------------------------------------------- */

void MT_EnableEvent(MT_Event* r, int flags)
{
    MTI_Event* e = NULL;
    MTI_EventQueue* s = r->event_queue;
    int regnum;

#ifndef _WIN32

    if (flags & MT_EVENT_HANDLE) {
        flags |= MT_EVENT_READ;
    }

#endif

    switch (r->type) {
    case MTI_CLIENT_SOCKET:
    case MTI_SERVER_SOCKET:

        dv_find(s->socket_regs, r, &regnum);

        assert(regnum >= 0);
        e = &s->events.data[regnum];

        if (flags & MT_EVENT_READ) {
            e->events |= FD_READ;
        }

        if (flags & MT_EVENT_WRITE) {
            e->events |= FD_WRITE;
        }

        if (flags & MT_EVENT_ACCEPT) {
            e->events |= FD_ACCEPT;
        }

        if (flags & MT_EVENT_CLOSE) {
            e->events |= FD_CLOSE;
        }

        break;

#ifdef _WIN32
    case MTI_HANDLE:

        if (flags & MT_EVENT_HANDLE && !r->enabled) {
            dv_append2(&s->handles, &r->handle, 1);
            dv_append2(&s->handle_regs, &r, 1);
            r->enabled = true;
        }

        break;
#endif

    case MTI_TICK:

        if (flags & MT_EVENT_TICK && !r->enabled) {
            int i;
            r->next_tick = MT_CurrentTime() + r->period;
            i = FindFirstGreaterOrEqual(&s->tick_regs, r->next_tick);
            dv_insert2(&s->tick_regs, i, &r, 1);
            r->enabled = true;
        }

        break;

    case MTI_IDLE:

        if (flags & MT_EVENT_IDLE && !r->enabled) {
            dv_append2(&s->idle_regs, &r, 1);
            r->enabled = true;
        }

        break;
    }
}

/* ------------------------------------------------------------------------- */

void MT_DisableEvent(MT_Event* r, int flags)
{
    MTI_Event* e = NULL;
    MTI_EventQueue* s = r->event_queue;
    int regnum;

#ifndef _WIN32

    if (flags & MT_EVENT_HANDLE) {
        flags |= MT_EVENT_READ;
    }

#endif

    switch (r->type) {
    case MTI_CLIENT_SOCKET:
    case MTI_SERVER_SOCKET:

        dv_find(s->socket_regs, r, &regnum);

        assert(regnum >= 0);
        e = &s->events.data[regnum];

        if (flags & MT_EVENT_READ) {
            e->events &= ~FD_READ;
        }

        if (flags & MT_EVENT_WRITE) {
            e->events &= ~FD_WRITE;
        }

        if (flags & MT_EVENT_ACCEPT) {
            e->events &= ~FD_ACCEPT;
        }

        if (flags & MT_EVENT_CLOSE) {
            e->events &= ~FD_CLOSE;
        }

        e->revents = 0;

        break;

#ifdef _WIN32
    case MTI_HANDLE:

        dv_find(s->handle_regs, r, &regnum);

        if ((flags & MT_EVENT_HANDLE) && r->enabled && regnum >= 0) {

            dv_erase(&s->handle_regs, regnum, 1);
            dv_erase(&s->handles, regnum + s->socket_regs.size, 1);
            r->enabled = false;
        }

        break;
#endif

    case MTI_TICK:

        if ((flags & MT_EVENT_TICK) && r->enabled) {
            int i = FindFirstGreaterOrEqual(&s->tick_regs, r->next_tick);

            while (s->tick_regs.data[i] != r) {
                assert(r->next_tick == s->tick_regs.data[i]->next_tick);
                i++;
            }

            dv_erase(&s->tick_regs, i, 1);
            r->enabled = false;
        }

        break;

    case MTI_IDLE:

        dv_find(s->idle_regs, r, &regnum);

        if ((flags & MT_EVENT_IDLE) && r->enabled && regnum >= 0) {

            if (s->next_idle > regnum) {
                s->next_idle--;
            }

            dv_erase(&s->idle_regs, regnum, 1);
            r->enabled = false;
        }

        break;
    }
}

/* ------------------------------------------------------------------------- */

void MT_ResetEvent(MT_Event* r)
{
    MTI_Event* e = NULL;
    MTI_EventQueue* s;
    int regnum;

    if (r == NULL || !r->enabled) {
        return;
    }

    s = r->event_queue;

    switch (r->type) {
    case MTI_CLIENT_SOCKET:
    case MTI_SERVER_SOCKET:
        dv_find(s->socket_regs, r, &regnum);

        assert(regnum >= 0);
        e = &s->events.data[regnum];
        e->revents = 0;
        break;

    case MTI_TICK:
        MT_DisableEvent(r, MT_EVENT_TICK);
        MT_EnableEvent(r, MT_EVENT_TICK);
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------------- */

void MT_FreeEvent(MT_Event* r)
{
    if (r) {
        MTI_EventQueue* s = r->event_queue;
        int regnum;

        switch (r->type) {
        case MTI_CLIENT_SOCKET:
        case MTI_SERVER_SOCKET:

            dv_find(s->socket_regs, r, &regnum);

            if (regnum >= 0) {
                dv_erase(&s->socket_regs, regnum, 1);
                dv_erase(&s->events, regnum, 1);

#           ifdef _WIN32
                dv_erase(&s->handles, regnum, 1);
                CloseHandle(r->handle);

                if (s->next_event == regnum) {
                    s->next_event = -1;
                }
#           else
                if (s->next_event > regnum) {
                    s->next_event--;
                }
#           endif

            }

            break;

#   ifdef _WIN32
        case MTI_HANDLE:

            dv_find(s->handle_regs, r, &regnum);

            if (r->enabled && regnum >= 0) {
                dv_erase(&s->handle_regs, regnum, 1);
                dv_erase(&s->handles, regnum + s->socket_regs.size, 1);
            }

            break;
#   endif

        case MTI_TICK:

            if (r->enabled) {
                int i = FindFirstGreaterOrEqual(&s->tick_regs, r->next_tick);

                /* i points to the first tick_reg with a next_tick == r->next_tick */
                while (s->tick_regs.data[i] != r) {
                    assert(r->next_tick == s->tick_regs.data[i]->next_tick);
                    i++;
                }

                dv_erase(&s->tick_regs, i, 1);
            }

            break;

        case MTI_IDLE:

            dv_find(s->idle_regs, r, &regnum);

            if (r->enabled && regnum >= 0) {
                dv_erase(&s->idle_regs, regnum, 1);

                if (s->next_idle > regnum) {
                    s->next_idle--;
                }
            }

            break;
        }

        free(r);
    }
}
