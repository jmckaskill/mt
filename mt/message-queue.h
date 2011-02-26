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

#include "mt-internal.h"
#include "queue.h"
#include "event-queue.h"
#include <mt/message.h>

struct MT_MessageQueue {
    MT_AtomicInt            ref;
    MTI_AtomicQueue         queue;
    MT_AtomicInt            woken;
    VoidDelegate            wakeup;
    MTI_EventQueue          event_queue;
};

typedef struct MTI_MessageHead MTI_MessageHead;
typedef struct MTI_MessagePart MTI_MessagePart;

struct MTI_MessagePart {
    MTI_AtomicQueueItem qitem;
    MT_Delegate_void    dlg;
    MT_WeakData*        weak_data;
    MTI_MessageHead*    header;
};

struct MTI_MessageHead {
    MT_AtomicInt        ref;
    MT_Callback         destroy_argument;
    /* padding to align to 8 */
    /* argument data */
    /* padding to align to 8 */
    /* message parts */
};

/* size aligned to 8 */
#define MTI_MESSAGE_HEAD_SIZE ((sizeof(MTI_MessageHead) + 7) & ~7)

MT_MessageQueue* MTI_CreateCurrentMessageQueue(void);
void MTI_DestroyMessageQueue(MT_MessageQueue* s);

