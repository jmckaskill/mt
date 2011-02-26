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

#include <string.h>
#include <time.h>

/* -------------------------------------------------------------------------- */

void MT_DateString(d_Vector(char)* out, MT_Time t)
{
    MT_BrokenDownTime tm;
    if (!MT_ToBrokenDownTime(t, &tm)) {
        dv_print(out, "%d-%02d-%02d", tm.year, tm.month, tm.date);
    }
}

/* -------------------------------------------------------------------------- */

void MT_TimeString(d_Vector(char)* out, MT_Time t)
{
    MT_BrokenDownTime tm;

    if (!MT_ToBrokenDownTime(t, &tm)) {
        dv_print(out, "%02d:%02d:%02d.%03d%03dZ",
                      tm.hour,
                      tm.minute,
                      tm.second,
                      tm.millisecond,
                      tm.microsecond);
    }
}

/* -------------------------------------------------------------------------- */

void MT_DateTimeString(d_Vector(char)* out, MT_Time t)
{
    MT_BrokenDownTime tm;

    if (!MT_ToBrokenDownTime(t, &tm)) {
        dv_print(out, "%d-%02d-%02d %02d:%02d:%02d.%03d%03dZ",
                      tm.year,
                      tm.month,
                      tm.date,
                      tm.hour,
                      tm.minute,
                      tm.second,
                      tm.millisecond,
                      tm.microsecond);
    }
}

/* -------------------------------------------------------------------------- */

void MT_FormatTimeString(d_Vector(char)* out, const char* fmt, MT_Time t)
{
    size_t used;
    struct tm tm;
    MT_BrokenDownTime btime;
    char* buf = dv_append_buffer(out, 1024);
    MT_ToBrokenDownTime(t, &btime);

    tm.tm_sec = btime.second;
    tm.tm_min = btime.minute;
    tm.tm_hour = btime.hour;
    tm.tm_mday = btime.date;
    tm.tm_wday = btime.week_day;
    tm.tm_mon = btime.month - 1;
    tm.tm_year = btime.year - 1900;

    used = strftime(buf, 1024, fmt, &tm);
    dv_erase_end(out, 1024 - used);
}

