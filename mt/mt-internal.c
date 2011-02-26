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
#include <mt/time.h>
#include <dmem/char.h>
#include <stdio.h>

#if defined _WIN32 && !defined NDEBUG
#   include <crtdbg.h>
#endif

#ifdef _WIN32
#   include <windows.h>
#else
#   include <pthread.h>
#endif

/* ------------------------------------------------------------------------- */

void MT_Log(const char* format, ...)
{
    d_Vector(char) str = DV_INIT;
    va_list ap;
    MT_BrokenDownTime t;
    MT_ToBrokenDownTime(MT_CurrentTime(), &t);

#ifdef _WIN32
    dv_print(&str, "[libmt %02d:%02d:%02d.%03d %d] ", t.hour, t.minute, t.second, t.millilecond, GetCurrentThreadId());
#elif __linux__
    if (MT_LOG_COLOR) {
        dv_append(&str, C("\033[1;30m"));
    }

    dv_print(&str, "[libmt %02d:%02d:%02d.%03d %p] ", t.hour, t.minute, t.second, t.millisecond, (void*) pthread_self());

    if (MT_LOG_COLOR) {
        dv_append(&str, C("\033[m"));
    }
#endif

    va_start(ap, format);
    dv_vprint(&str, format, ap);
    va_end(ap);

    dv_append(&str, C("\n"));

#if defined _WIN32 && !defined _WIN32_WCE
    OutputDebugStringA(str.data);
#else
    fwrite(str.data, 1, str.size, stderr);
#endif

    dv_free(str);
}

