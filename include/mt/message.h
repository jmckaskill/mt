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
#include <mt/ref.h>
#include <mt/lock.h>
#include <dmem/list.h>

/* ------------------------------------------------------------------------- */

struct MT_Object {
    /* internal */
    MT_WeakData*        weak_data;
    MT_MessageQueue*    message_queue;
};

struct MT_WeakData {
    /* internal */
    MT_Object*          object;
    MT_MessageQueue*    message_queue;
    MT_AtomicInt        ref;
    MT_AtomicInt        msg_ref;
};

enum MT_SendType {
    MT_SEND_AUTO,
    MT_SEND_PROXIED,
    MT_SEND_DIRECT
};

typedef enum MT_SendType MT_SendType;

/* ------------------------------------------------------------------------- */

/* Operations on message queues */

MT_API MT_MessageQueue* MT_CurrentMessageQueue(void);
MT_API void MT_SetCurrentMessageQueue(MT_MessageQueue* s);

MT_API MT_MessageQueue* MT_NewMessageQueue(void);
MT_API void MT_FreeMessageQueue(MT_MessageQueue* s);
MT_API void MT_ProcessMessageQueue(MT_MessageQueue* s);
MT_API void MT_SetMessageQueueWakeup(MT_MessageQueue* s, VoidDelegate wakeup);

/* ------------------------------------------------------------------------- */

/* Operations on objects */

MT_API void MT_InitObject(MT_Object* s);
MT_API void MT_InitObject2(MT_Object* s, MT_MessageQueue* mq);
MT_API void MT_DestroyObject(MT_Object* s);

#define MT_RefWeakData(pwd) MT_Ref(pwd)
#define MT_DerefWeakData(pwd) MT_Deref(pwd, MT_FreeWeakData(pwd))

MT_API MT_WeakData* MT_GetWeakData(const MT_Object* s);
MT_API void MT_FreeWeakData(MT_WeakData* s);
MT_API bool MT_IsSynchronous(MT_WeakData* s);

/* ------------------------------------------------------------------------- */

/* Operations on channels */

#define MT_Pipe(name) MT_Pipe_##name
#define MT_NULL_PIPE {NULL, NULL, NULL}

#define MT_CheckDlg(FUNC, OBJ, PDATA)   ((void) (0 && ((FUNC)(OBJ, PDATA), 0)))
#define MT_CheckChData(PCH, PDATA)      ((void) (0 && ((PCH)->h.init(NULL, PDATA), 0)))

#define MT_InitPipe(name, PCH)      MT_InitPipe_##name(PCH)
#define MT_DestroyPipe(PCH)         MT_DerefWeakData((PCH)->weak_data)

#define MT_SetPipe(PCH, FUNC, OBJ)                                              \
    do {                                                                        \
        MT_WeakData* oldwd = (PCH)->weak_data;                                  \
        VoidDelegate_cb* func = (VoidDelegate_cb*) &(PCH)->dlg.func;            \
        MT_CheckDlg(FUNC, OBJ, (PCH)->h.typecheck_data);                        \
        *func = (VoidDelegate_cb) (FUNC);                                       \
        (PCH)->dlg.obj = OBJ;                                                   \
        (PCH)->weak_data = MT_GetWeakData((MT_Object*) (PCH)->dlg.obj);         \
        MT_RefWeakData((PCH)->weak_data);                                       \
        MT_DerefWeakData(oldwd);                                                \
    } while (0)


#define MT_CopyPipe(PTO, FROM)                                                  \
    do {                                                                        \
        MT_WeakData* oldwd = (PTO)->weak_data;                                  \
        *(PTO) = FROM;                                                          \
        MT_RefWeakData((PTO)->weak_data);                                       \
        MT_DerefWeakData(oldwd);                                                \
    } while(0)

#define MT_Send(PCH, PDATA)         (MT_BaseSend(PCH, PDATA), MT_CheckChData(PCH, PDATA))
#define MT_SendProxied(PCH, PDATA)  (MT_BaseSendProxied(PCH, PDATA), MT_CheckChData(PCH, PDATA))

#define MT_SendDirect(PCH, PDATA)                                               \
    do {                                                                        \
        if ((PCH)->weak_data && (PCH)->weak_data->object) {                     \
            CALL_DELEGATE_1((PCH)->dlg, PDATA);                                 \
        }                                                                       \
    } while(0)


MT_API void MT_BaseSend(const void* channel, const void* argument);
MT_API void MT_BaseSendProxied(const void* channel, const void* argument);

/* ------------------------------------------------------------------------- */

/* Operations on signals */

#define MT_Signal(name)                     MT_Signal_##name

#define MT_CheckSigData(PSIG, PDATA)        ((void) (0 && ((PSIG)->h.init(NULL, PDATA), 0)))
#define MT_CheckSig(PSIG)                   ((void) (0 && (PSIG)->targets))
#define MT_CheckSigCh(PSIG, PCH)            ((void) (0 && (PSIG)->h.init == (PCH)->h.init))
#define MT_CheckSigFunc(PSIG, FUNC, OBJ)    MT_CheckDlg(FUNC, OBJ, (PSIG)->h.typecheck_data)

#define MT_InitSignal(name, PSIG)           MT_InitSignal_##name(PSIG)
#define MT_DestroySignal(PSIG)              (MT_BaseDestroySignal((PSIG)->targets), MT_DestroyMutex(&(PSIG)->lock))
#define MT_Emit(PSIG, PDATA)                (MT_BaseEmit(PSIG, PDATA), MT_CheckSigData(PSIG, PDATA))

#define MT_Connect(PSIG, FUNC, OBJ)         (MT_BaseConnect2(PSIG, (MT_MessageCallback) (FUNC), OBJ, MT_SEND_AUTO), MT_CheckSigFunc(PSIG, FUNC, OBJ))
#define MT_Connect2(PSIG, PCH)              (MT_BaseConnect(PSIG, PCH, MT_SEND_AUTO), MT_CheckSigCh(PSIG, PCH))
#define MT_Connect3(PSIG, PCH, TYPE)        (MT_BaseConnect(PSIG, PCH, TYPE), MT_CheckSigCh(PSIG, PCH))

#define MT_Disconnect(PSIG, FUNC, OBJ)      (MT_BaseDisconnect2(PSIG, FUNC, OBJ), MT_CheckSigFunc(FUNC, OBJ, PSIG))
#define MT_Disconnect2(PSIG, PCH)           (MT_BaseDisconnect(PSIG, PCH), MT_CheckSigCh(PSIG, PCH))

MT_API void MT_BaseDestroySignal(MTI_DelegateVector* targets);
MT_API void MT_BaseConnect(void* sig, const void* channel, MT_SendType type);
MT_API void MT_BaseConnect2(void* sig, MT_MessageCallback func, void* obj, MT_SendType type);
MT_API void MT_BaseDisconnect(void* sig, const void* channel);
MT_API void MT_BaseDisconnect2(void* sig, MT_MessageCallback func, void* obj);
MT_API void MT_BaseEmit(const void* sig, const void* argument);

/* ------------------------------------------------------------------------- */

#define MT_DECLARE_MESSAGE_TYPE_2(name, data_t, SIZE, INIT, DESTROY)            \
    DECLARE_DELEGATE_1(MT_Delegate_##name, void, const data_t*);                \
                                                                                \
    struct MTI_PipeHeader_##name {                                              \
        int data_size;                                                          \
        void (*init)(data_t*, const data_t*);                                   \
        void (*destroy)(data_t*);                                               \
        const data_t* typecheck_data;                                           \
    };                                                                          \
                                                                                \
    struct MT_Pipe(name) {                                                      \
        struct MTI_PipeHeader_##name h;                                         \
        MT_Delegate_##name dlg;                                                 \
        MT_WeakData* weak_data;                                                 \
    };                                                                          \
                                                                                \
    struct MT_Signal(name) {                                                    \
        struct MTI_PipeHeader_##name h;                                         \
        MTI_DelegateVector* targets;                                            \
        MT_Mutex lock;                                                          \
    };                                                                          \
                                                                                \
    typedef struct MT_Pipe(name) MT_Pipe(name);                                 \
    typedef struct MT_Signal(name) MT_Signal(name);                             \
                                                                                \
    static void MT_InitPipe_##name(MT_Pipe(name)* pch) {                        \
        pch->dlg.func = NULL;                                                   \
        pch->weak_data = NULL;                                                  \
        pch->h.init = INIT;                                                     \
        pch->h.destroy = DESTROY;                                               \
        pch->h.data_size = ((SIZE) + 7) & ~7; /* align to 8 */                  \
    }                                                                           \
                                                                                \
    static void MT_InitSignal_##name(MT_Signal(name)* psig) {                   \
        MT_InitMutex(&psig->lock);                                              \
        psig->targets = NULL;                                                   \
        psig->h.init = INIT;                                                    \
        psig->h.destroy = DESTROY;                                              \
        psig->h.data_size = ((SIZE) + 7) & ~7; /* align to 8 */                 \
    }                                                                           \
                                                                                \
    struct MT_ConsumeSemicolon_##name

#define MT_DECLARE_MESSAGE_TYPE(name, data_t, INIT, DESTROY)                    \
    MT_DECLARE_MESSAGE_TYPE_2(name, data_t, sizeof(data_t), INIT, DESTROY)

MT_DECLARE_MESSAGE_TYPE_2(void, void, 0, NULL, NULL);
MT_DECLARE_MESSAGE_TYPE(int, int, NULL, NULL);

