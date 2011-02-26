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

#define _POSIX_SOURCE

#include "process-unix.h"
#include "wakeup-event.h"
#include "event-queue.h"
#include "message-queue.h"
#include <mt/thread.h>
#include <mt/event.h>
#include <mt/lock.h>
#include <dmem/hash.h>
#include <dmem/vector.h>
#include <string.h>
#include <assert.h>

DHASH_INIT_INT(Child, MT_Process*);

#if !defined _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

static MT_Mutex g_lock = MT_MUTEX_INITIALIZER;
static MT_Thread* g_thread;
static d_IntHash(Child) g_table;
static MTI_WakeupEvent g_wakeup;
static struct sigaction g_oldsig;

/* ------------------------------------------------------------------------- */

/* Called on child exit thread */
static void ProcessChildExit(void* u)
{
    int status;
    pid_t pid;
    int exit_code = -1;
    size_t idx;

    (void) u;

    pid = waitpid(-1, &status, WNOHANG);

    if (pid < 0) {
        return;
    }

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    }

    MT_Lock(&g_lock);

    if (dhi_find(&g_table, pid, &idx)) {

        g_table.vals[idx]->pid = -1;
        dh_erase(&g_table, idx);

        MT_Emit(&g_table.vals[idx]->on_exit, &exit_code);
    }

    MT_Unlock(&g_lock);
}

/* ------------------------------------------------------------------------- */

/* Called asynchronously on arbitrary thread - signal handler */
static void OnSignal(int signum)
{
    MTI_TriggerWakeupEvent(&g_wakeup);
    (void) signum;
}

/* ------------------------------------------------------------------------- */

static int ChildOnExitThread(void* u)
{
    (void) u;
    MT_RunEventLoop();
    sigaction(SIGCHLD, &g_oldsig, NULL);
    MTI_DestroyWakeupEvent(&g_wakeup);
    return 0;
}

/* ------------------------------------------------------------------------- */

MT_Process* MT_Process_New(void)
{
    MT_Process* s = NEW(MT_Process);
    MT_InitSignal(int, &s->on_exit);
    s->pid = -1;

    MT_Lock(&g_lock);

    if (!g_thread) {
        struct sigaction sig;

        g_thread = MT_NewThread("SIGCHLD Process Thread");
        MT_BeginThreadInit(g_thread);

        memset(&sig, 0, sizeof(sig));
        sig.sa_handler = &OnSignal;
        sig.sa_flags = SA_NOCLDSTOP;

        MTI_InitWakeupEvent(
                &g_wakeup,
                &MTI_CreateCurrentMessageQueue()->event_queue,
                BindVoid(&ProcessChildExit, NULL));

        sigaction(SIGCHLD, &sig, &g_oldsig);

        MT_StartThread(g_thread, BindInt(&ChildOnExitThread, NULL));
    }

    MT_Unlock(&g_lock);

    return s;
}

/* ------------------------------------------------------------------------- */

void MT_Process_Free(MT_Process* s)
{
    if (s) {
        int i;

        MT_Lock(&g_lock);

        dhi_remove(&g_table, s->pid);

        if (dh_size(&g_table) == 0) {
            MT_FreeThread(g_thread);
            g_thread = NULL;
        }

        MT_Unlock(&g_lock);

        for (i = 0; i < s->arguments.size; i++) {
            d_Vector(char) str = DV_INIT;
            str.data = s->arguments.data[i];
            dv_free(str);
        }

        MT_DestroySignal(&s->on_exit);
        dv_free(s->arguments);
        dv_free(s->working_directory);
        free(s);
    }
}

/* ------------------------------------------------------------------------- */

void MT_Process_SetWorkingDirectory(MT_Process* s, const char* dir)
{
    dv_set(&s->working_directory, dv_char(dir));
}

/* ------------------------------------------------------------------------- */

void MT_Process_AddArgument(MT_Process* s, const char* arg)
{
    d_Vector(char) dest = DV_INIT;
    dv_set(&dest, dv_char(arg));
    dv_append2(&s->arguments, &dest.data, 1);
}

/* ------------------------------------------------------------------------- */

int MT_Process_Start(MT_Process* s, const char* app)
{
    MT_Lock(&g_lock);

    s->pid = fork();

    if (s->pid == 0) {
        /* Success in child */
        dv_insert2(&s->arguments, 0, &app, 1);

        if (s->working_directory.size) {
            chdir(s->working_directory.data);
        }

        execv(app, (char**) s->arguments.data);
        exit(-1);

    } else if (s->pid == -1) {
        /* Error in parent */
        MT_Unlock(&g_lock);
        return -1;

    } else {
        /* Success in parent */
        dhi_set(&g_table, s->pid, s);
        MT_Unlock(&g_lock);
        return 0;
    }
}

/* ------------------------------------------------------------------------- */

int MT_Process_Spawn(const char* app)
{
    switch (fork()) {
    case -1:
        /* Error in parent */
        return -1;

    case 0:
        /* Success in parent */
        return 0;

    default:
        execl(app, app, NULL);
        exit(-1);
    }
}

/* ------------------------------------------------------------------------- */

void MT_ProcessName(d_Vector(char)* out, int64_t pid)
{
    (void) out;
    (void) pid;
}

#endif

