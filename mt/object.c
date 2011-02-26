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

#include "mt-internal.h"
#include <mt/message.h>
#include "message-queue.h"
#include "queue.h"
#include <mt/lock.h>
#include <mt/message.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

void MT_InitObject(MT_Object* s)
{
    s->weak_data = NULL;
    s->message_queue = MTI_CreateCurrentMessageQueue();
    MT_Ref(s->message_queue);
}

/* ------------------------------------------------------------------------- */

void MT_InitObject2(MT_Object* s, MT_MessageQueue* q)
{
    s->weak_data = NULL;
    s->message_queue = q;
    MT_Ref(s->message_queue);
}

/* ------------------------------------------------------------------------- */

void MT_FreeWeakData(MT_WeakData* s)
{
    MT_Deref(s->message_queue, MTI_DestroyMessageQueue(s->message_queue));
    MT_Deref2(s, msg_ref, free(s));
}

/* ------------------------------------------------------------------------- */

void MT_DestroyObject(MT_Object* s)
{
    if (s->weak_data) {
        s->weak_data->object = NULL;
        MT_DerefWeakData(s->weak_data);
    } else {
        MT_Deref(s->message_queue, MTI_DestroyMessageQueue(s->message_queue));
    }
}

/* ------------------------------------------------------------------------- */

MT_WeakData* MT_GetWeakData(const MT_Object* sc)
{
    MT_Object* s = (MT_Object*) sc;

    if (s->weak_data == NULL) {

        MT_WeakData* p = (MT_WeakData*) malloc(sizeof(MT_WeakData));

        p->message_queue = s->message_queue;
        p->object = s;
        p->ref = 1;
        p->msg_ref = 1;

        /* Race to see if we are the first to set ptr, if we aren't the first
         * then we use their version and free our version.
         */
        if (MT_AtomicSetPtrFrom(&s->weak_data, NULL, p) != NULL) {
            /* We lost the race */
            free(p);
        }
    }

    return s->weak_data;
}

/* ------------------------------------------------------------------------- */

bool MT_IsSynchronous(MT_WeakData* s)
{
    return s->message_queue == MT_CurrentMessageQueue();
}


