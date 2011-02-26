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

#include "message-queue.h"
#include "queue.h"
#include "mt-signal.h"
#include <mt/atomic.h>
#include <mt/thread.h>
#include <assert.h>

/* ------------------------------------------------------------------------- */

static MT_ThreadStorage g_message_queue = MT_THREAD_STORAGE_INITIALIZER;

void MT_SetCurrentMessageQueue(MT_MessageQueue* s)
{
    MT_SetThreadStorage(&g_message_queue, s);
}

MT_MessageQueue* MT_CurrentMessageQueue(void)
{
    return (MT_MessageQueue*) MT_GetThreadStorage(&g_message_queue);
}

/* ------------------------------------------------------------------------- */

DVECTOR_INIT(MessageQueue, MT_MessageQueue*);
static d_Vector(MessageQueue) g_queues;
static MT_Mutex g_queues_lock = MT_MUTEX_INITIALIZER;

static void FreeQueues(void)
{ 
    int i;
    for (i = 0; i < g_queues.size; i++) {
        MT_FreeMessageQueue(g_queues.data[i]); 
    }

    dv_free(g_queues);
}

MT_MessageQueue* MTI_CreateCurrentMessageQueue(void)
{
    static int atexit_ret = -1;
    MT_MessageQueue* q = MT_CurrentMessageQueue();

    if (q) {
        return q;
    }

    q = MT_NewMessageQueue();
    MT_SetCurrentMessageQueue(q);

    MT_Lock(&g_queues_lock);
    dv_append2(&g_queues, &q, 1);

    if (atexit_ret) {
        atexit_ret = atexit(&FreeQueues);
    }

    MT_Unlock(&g_queues_lock);

    return q;
}


/* ------------------------------------------------------------------------- */

MT_MessageQueue* MT_NewMessageQueue(void)
{
    MT_MessageQueue* s = NEW(MT_MessageQueue);
    MT_Ref(s);
    MTI_InitEventQueue(&s->event_queue, s);
    return s;
}

/* ------------------------------------------------------------------------- */

static MTI_MessagePart* CreateMessage(struct MTI_PipeHeader_void* ph, int parts, const void* data)
{
    int i;

    int alloc = MTI_MESSAGE_HEAD_SIZE + ph->data_size + (parts * sizeof(MTI_MessagePart));
    MTI_MessageHead* h = (MTI_MessageHead*) malloc(alloc);
    MTI_MessagePart* p = (MTI_MessagePart*) ((char*) h + MTI_MESSAGE_HEAD_SIZE + ph->data_size);

    h->ref = 1;
    h->destroy_argument = ph->destroy;

    if (ph->init) {
        ph->init((char*) h + MTI_MESSAGE_HEAD_SIZE, data);
    } else {
        memcpy((char*) h + MTI_MESSAGE_HEAD_SIZE, data, ph->data_size);
    }

    for (i = 0; i < parts; i++) {
        p[i].header = h;
    }

    return p;
}

static void DerefMessage(MTI_MessageHead* h)
{
    if (h && MT_AtomicDecrement(&h->ref) == 0) {

        if (h->destroy_argument) {
            h->destroy_argument((char*) h + MTI_MESSAGE_HEAD_SIZE);
        }

        free(h);
    }
}

static void FreeQueuedMessages(MT_MessageQueue* s)
{
    for (;;) {

        MTI_MessagePart* m;
        MTI_MessageHead* h;
        MTI_AtomicQueueItem* item = MTI_AtomicQueue_Consume(&s->queue);

        if (!item) {
            break;
        }

        m = container_of(item, MTI_MessagePart, qitem);
        h = m->header;

        MT_Deref2(m->weak_data, msg_ref, free(m->weak_data));
        DerefMessage(h);
    }
}

void MTI_DestroyMessageQueue(MT_MessageQueue* s)
{
    FreeQueuedMessages(s);
    assert(!s->queue.first && !s->queue.last);
    free(s);
}

void MT_FreeMessageQueue(MT_MessageQueue* s)
{
    if (s) {
        FreeQueuedMessages(s);
        MTI_DestroyEventQueue(&s->event_queue);
        MT_Deref(s, MTI_DestroyMessageQueue(s));
    }
}

/* ------------------------------------------------------------------------- */

void MT_SetMessageQueueWakeup(MT_MessageQueue* s, VoidDelegate wakeup)
{
    s->wakeup = wakeup;

    if (s->queue.first) {
        MT_AtomicSet(&s->woken, 1);
        CALL_DELEGATE_0(s->wakeup);
    }
}

/* ------------------------------------------------------------------------- */

void MT_ProcessMessageQueue(MT_MessageQueue* s)
{
    MT_AtomicSet(&s->woken, 0);

    for (;;) {

        MTI_MessagePart* m;
        MTI_MessageHead* h;
        MTI_AtomicQueueItem* item = MTI_AtomicQueue_Consume(&s->queue);

        if (!item) {
            break;
        }

        m = container_of(item, MTI_MessagePart, qitem);
        h = m->header;

        if (m->weak_data->object) {
            CALL_DELEGATE_1(m->dlg, (char*) h + MTI_MESSAGE_HEAD_SIZE);
        }

        MT_Deref2(m->weak_data, msg_ref, free(m->weak_data));
        DerefMessage(h);
    }
}

/* ------------------------------------------------------------------------- */

static void ProxiedSend(MTI_MessagePart* p, MT_Delegate_void dlg, MT_WeakData* weak_data)
{
    MT_MessageQueue* s = weak_data->message_queue;

    p->dlg = dlg;
    p->weak_data = weak_data;
    p->qitem.next = NULL;

    MT_Ref2(p->weak_data, msg_ref);

    MTI_AtomicQueue_Produce(&s->queue, &p->qitem);

    if (MT_AtomicSetFrom(&s->woken, 0, 1) == 0) {
        CALL_DELEGATE_0(s->wakeup);
    }
}

/* ------------------------------------------------------------------------- */

void MT_BaseSend(const void* channel, const void* argument)
{
    MT_Pipe(void)* pch = (MT_Pipe(void)*) channel;
    MT_WeakData* p = pch->weak_data;

    if (p == NULL || p->object == NULL) {
        /* Do nothing */
    } else if (p->message_queue == MT_CurrentMessageQueue()) {
        CALL_DELEGATE_1(pch->dlg, (void*) argument);
    } else  {
        MTI_MessagePart* p = CreateMessage( &pch->h, 1, argument);
        ProxiedSend(p, pch->dlg, pch->weak_data);
    }
}

/* ------------------------------------------------------------------------- */

void MT_BaseSendProxied(const void* channel, const void* argument)
{
    MT_Pipe(void)* pch = (MT_Pipe(void)*) channel;

    if (pch->weak_data && pch->weak_data->object) {
        MTI_MessagePart* p = CreateMessage(&pch->h, 1, argument);
        ProxiedSend(p, pch->dlg, pch->weak_data);
    }
}

/* ------------------------------------------------------------------------- */

void MT_BaseEmit(const void* psig, const void* argument)
{
    MT_Signal(void)* sig = (MT_Signal(void)*) psig;
    MTI_DelegateVector* vec;
    MT_MessageQueue* cur_queue;
    MTI_MessageHead* header = NULL;
    MTI_MessagePart* msg = NULL;
    int i;

    if (!sig->targets) {
        return;
    }

    /* Grab a reference to the list of targets and then drop the lock so that
     * we aren't holding the lock whilst calling the targets (some of which may
     * be synchronous and may take infinite time or would otherwise deadlock on
     * this signal).
     */

    MT_Lock(&sig->lock);
    vec = sig->targets;
    MT_Ref(vec);
    MT_Unlock(&sig->lock);

    if (!vec) {
        return;
    }

    cur_queue = MT_CurrentMessageQueue();

    for (i = 0; i < vec->size; i++) {
        MTI_SignalReg* r = &vec->regs[i];
       
        if (r->weak_data->object == NULL) {
            /* Do nothing */
        } else if (r->type == MT_SEND_DIRECT || (r->type == MT_SEND_AUTO && r->weak_data->message_queue == cur_queue)) {
            CALL_DELEGATE_1(r->dlg, (void*) argument);
        } else {
            if (!msg) {
                /* Create a large enough message to hold parts for all the
                 * remaining registrations in case they are all proxied.
                 */
                msg = CreateMessage(&sig->h, vec->size - i, argument);
                header = msg->header;
            }

            MT_AtomicIncrement(&header->ref);
            ProxiedSend(msg++, r->dlg, r->weak_data);
        }
    }

    DerefMessage(header);
    MT_Deref(vec, MTI_FreeDelegateVector(vec));
}






