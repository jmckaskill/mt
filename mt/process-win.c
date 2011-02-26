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


#include "process-win.h"
#include <mt/event.h>
#include <dmem/wchar.h>

#ifdef _WIN32
#include <windows.h>
#include <Psapi.h>

/* ------------------------------------------------------------------------- */

static void AppendExeDirectory(d_Vector(wchar)* s)
{
    uint16_t *dest, *filename;
    DWORD used;

    dest = dv_append_buffer(s, 1024);
    used = GetModuleFileNameW(NULL, dest, 1024);
    dv_erase_end(s, 1024 - used - 1);
    filename = wcsrchr(s->data, '\\');
    dv_erase_end(s, s->data + s->size - filename);
    dv_append(s, W("\\"));
}

/* ------------------------------------------------------------------------- */

static void AppendCurrentDirectory(d_Vector(wchar)* s)
{
#ifndef _WIN32_WCE
    DWORD req = GetCurrentDirectoryW(0, NULL) - 1;
    wchar_t* dest = dv_append_buffer(s, req);
    GetCurrentDirectoryW(req + 1, dest);
    dv_append(s, W("\\"));
#endif
}

/* ------------------------------------------------------------------------- */

static int IsRelativePath(const char* str)
{
    return str[0] != '/' && str[0] != '\\' && strchr(str, ':') == NULL;
}

/* ------------------------------------------------------------------------- */

static const char* Filename(const char* path)
{
    const char* end = path + strlen(path);

    while (end > path && *end != '\\' && *end != '/') {
        end--;
    }

    return end;
}

/* ------------------------------------------------------------------------- */

MT_Process* MT_Process_New(void)
{
    MT_Process* s = NEW(MT_Process);
    MT_InitSignal(int, &s->on_exit);
    s->process = INVALID_HANDLE_VALUE;
    return s;
}

/* ------------------------------------------------------------------------- */

void MT_Process_Free(MT_Process* s)
{
    if (s) {
        MT_FreeEvent(s->event);
        CloseHandle(s->process);
        MT_DestroySignal(&s->on_exit);
        dv_free(s->command_line);
        dv_free(s->working_directory);
        free(s);
    }
}

/* ------------------------------------------------------------------------- */

void MT_Process_SetWorkingDirectory(MT_Process* s, const char* dir)
{
    if (IsRelativePath(dir)) {
        AppendCurrentDirectory(&s->working_directory);
    }

    dv_append_from_utf8(&s->working_directory, dv_char(dir));
}

/* ------------------------------------------------------------------------- */

void MT_Process_AddArgument(MT_Process* s, const char* arg)
{
    dv_append(&s->command_line, W("\""));
    dv_append_from_utf8(&s->command_line, dv_char(arg));
    dv_append(&s->command_line, W("\" "));
}

/* ------------------------------------------------------------------------- */

static void OnExit(MT_Process* s)
{
    DWORD dwExitCode;
    int exit_code = -1;
    if (GetExitCodeProcess(s->process, &dwExitCode)) {
        exit_code = dwExitCode;
    }
    MT_Emit(&s->on_exit, &exit_code);
}

int MT_Process_Start(MT_Process* s, const char* app)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    d_Vector(wchar) abscmd = DV_INIT;
    d_Vector(wchar) cmdline = DV_INIT;
    const char* appfile;
    BOOL ret;
    DWORD dwCreationFlags = 0;

    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    dv_append(&cmdline, W("\""));
    dv_append_from_utf8(&cmdline, dv_char(app));
    dv_append(&cmdline, W("\" "));
    dv_append(&cmdline, s->command_line);

    appfile = Filename(app);

    /* Prepend the exe directory if the app is a relative path */
    if (IsRelativePath(app) && appfile != app) {
        AppendExeDirectory(&abscmd);
    }

    dv_append_from_utf8(&abscmd, dv_char(app));

    /* Default the extension to .exe if it doesn't already have one. */
    if (strchr(appfile, '.') == NULL) {
        dv_append(&abscmd, W(".exe"));
    }

#ifndef _WIN32_WCE
    dwCreationFlags = CREATE_NO_WINDOW | DETACHED_PROCESS;
#endif

    /* note s->working_directory will be NULL if it has not been appended to */
    ret = CreateProcessW(
              abscmd.data,          /* lpApplicationName */
              cmdline.data,         /* lpCommandLine */
              NULL,                 /* lpProcessAttributes */
              NULL,                 /* lpThreadAttributes */
              FALSE,                /* bInheritHandles */
              dwCreationFlags,      /* dwCreationFlags */
              NULL,                 /* lpEnvironment */
              s->working_directory.data, /* lpCurrentDirectory */
              &si,                  /* lpStartupInfo */
              &pi);                 /* lpProcessInformation */

    dv_free(abscmd);
    dv_free(cmdline);

    if (!ret) {
        return -1;
    }

    CloseHandle(pi.hThread);
    s->event = MT_NewHandleEvent(pi.hProcess, BindVoid(&OnExit, s));

    return 0;
}

/* ------------------------------------------------------------------------- */

int MT_Process_Spawn(const char* app)
{
    MT_Process* p = MT_Process_New();
    int ret = MT_Process_Start(p, app);
    MT_Process_Free(p);
    return ret;
}

/* ------------------------------------------------------------------------- */

typedef DWORD (WINAPI *GetProcessImageFileNameA_t)(HANDLE hProcess, LPSTR lpImageFileName, DWORD nSize);

void MT_ProcessName(d_Vector(char)* out, int64_t pid)
{
    HMODULE psapi = NULL;
    HANDLE process = INVALID_HANDLE_VALUE;
    GetProcessImageFileNameA_t GetProcessImageFileNameA;
    char *dest, *slash;
    int added;

    if (pid < 0) {
        goto end;
    }

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD) pid);
    if (process == INVALID_HANDLE_VALUE) {
        goto end;
    }

    psapi = LoadLibraryA("Psapi.dll");
    if (psapi == NULL) {
        goto end;
    }

    GetProcessImageFileNameA = (GetProcessImageFileNameA_t) GetProcAddress(psapi, "GetProcessImageFileNameA");
    if (!GetProcessImageFileNameA) {
        goto end;
    }

    dest = dv_append_buffer(out, MAX_PATH);
    added = GetProcessImageFileNameA(process, dest, MAX_PATH);
    dv_erase_end(out, MAX_PATH - added);
    if (added == 0) {
        goto end;
    }

    slash = strrchr(dest, '\\');
    if (slash) {
        dv_erase(out, dest - out->data, slash - out->data + 1);
    }

end:
    CloseHandle(process);
    FreeLibrary(psapi);
}

#endif
