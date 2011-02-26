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

#include "mt-signal.h"

/* ------------------------------------------------------------------------- */

void MTI_FreeDelegateVector(MTI_DelegateVector* vec)
{
    int i;

    for (i = 0; i < vec->size; i++) {
        MT_DerefWeakData(vec->regs[i].weak_data);
    }

    free(vec);
}

/* ------------------------------------------------------------------------- */

void MT_BaseDestroySignal(MTI_DelegateVector* targets)
{
    MT_Deref(targets, MTI_FreeDelegateVector(targets));
}

/* ------------------------------------------------------------------------- */

static void Connect(void* psig, MT_Delegate_void dlg, MT_WeakData* weak_data, MT_SendType type)
{
    MT_Signal(void)* sig = (MT_Signal(void)*) psig;
    size_t alloc = sizeof(MTI_DelegateVector);
    MTI_DelegateVector* new_vec;
    MTI_DelegateVector* cur_vec = sig->targets;

    MT_Lock(&sig->lock);

    if (cur_vec) {
        int i;

        alloc += cur_vec->size * sizeof(MTI_SignalReg);
        new_vec = (MTI_DelegateVector*) malloc(alloc);
        new_vec->ref = 1;
        new_vec->size = 0;

        for (i = 0; i < cur_vec->size; i++) {
            MTI_SignalReg* cur_reg = &cur_vec->regs[i];
            MTI_SignalReg* new_reg = &new_vec->regs[new_vec->size];

            if (cur_reg->weak_data->object == NULL) {
                MT_DerefWeakData(cur_reg->weak_data);
            } else {
                *new_reg = *cur_reg;
                MT_Ref(new_reg->weak_data);
                new_vec->size++;
            }
        }

    } else {
        new_vec = (MTI_DelegateVector*) malloc(alloc);
        new_vec->ref = 1;
        new_vec->size = 0;
    }

    new_vec->regs[new_vec->size].dlg = dlg;
    new_vec->regs[new_vec->size].weak_data = weak_data;
    new_vec->regs[new_vec->size].type = type;
    new_vec->size++;

    MT_Ref(weak_data);

    sig->targets = new_vec;
    MT_Deref(cur_vec, MTI_FreeDelegateVector(cur_vec));

    MT_Unlock(&sig->lock);
}

void MT_BaseConnect(void* psig, const void* channel, MT_SendType type)
{
    MT_Pipe(void)* pch = (MT_Pipe(void)*) channel;
    Connect(psig, pch->dlg, pch->weak_data, type);
}

void MT_BaseConnect2(void* psig, MT_MessageCallback func, void* obj, MT_SendType type)
{
    MT_Delegate_void dlg = NULL_DELEGATE;
    dlg.obj = obj;
    dlg.func = func;
    Connect(psig, dlg, MT_GetWeakData((MT_Object*) obj), type);
}

/* ------------------------------------------------------------------------- */

void MT_BaseDisconnect2(void* psig, MT_MessageCallback func, void* obj)
{
    MT_Signal(void)* sig = (MT_Signal(void)*) psig;
    MT_Lock(&sig->lock);

    if (sig->targets) {
        int i;
        MTI_DelegateVector *new_vec, *cur_vec;
        size_t alloc = sizeof(MTI_DelegateVector);

        cur_vec = sig->targets;
        new_vec = (MTI_DelegateVector*) malloc(alloc);

        /* -1 due to new_vec->regs being a [1] array */
        alloc += (cur_vec->size - 1) * sizeof(MTI_SignalReg);

        new_vec->ref = 1;
        new_vec->size = 0;

        for (i = 0; i < cur_vec->size; i++) {
            MTI_SignalReg* cur_reg = &cur_vec->regs[i];
            MTI_SignalReg* new_reg = &new_vec->regs[new_vec->size];

            if (cur_reg->weak_data->object == NULL || (cur_reg->dlg.func == func && cur_reg->dlg.obj == obj)) {
                MT_DerefWeakData(cur_reg->weak_data);
            } else {
                *new_reg = *cur_reg;
                MT_Ref(new_reg->weak_data);
                new_vec->size++;
            }
        }

        if (new_vec->size == 0) {
            MT_Deref(new_vec, MTI_FreeDelegateVector(new_vec));
            new_vec = NULL;
        }

        sig->targets = new_vec;
        MT_Deref(cur_vec, MTI_FreeDelegateVector(cur_vec));
    }

    MT_Unlock(&sig->lock);
}

void MT_BaseDisconnect(void* psig, const void* pch)
{
    MT_Pipe(void)* ch = (MT_Pipe(void)*) pch;
    MT_BaseDisconnect2(psig, ch->dlg.func, ch->dlg.obj);
}




/* ------------------------------------------------------------------------- */


