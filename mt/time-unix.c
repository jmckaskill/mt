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

/* To get localtime_r and timegm */
#define _BSD_SOURCE

#include "mt-internal.h"
#include <mt/time.h>

#ifndef _WIN32

#include <sys/time.h>
#include <time.h>

/* ------------------------------------------------------------------------- */

MT_Time MT_CurrentTime()
{
    struct timeval tv;
    MT_Time ret;

    if (gettimeofday(&tv, NULL)) {
        return MT_TIME_INVALID;
    }

    ret = MT_TIME_FROM_SECONDS(tv.tv_sec);
    ret += MT_TIME_FROM_US(tv.tv_usec);
    return ret;
}

/* ------------------------------------------------------------------------- */

int MT_ToBrokenDownTime(MT_Time t, MT_BrokenDownTime* b)
{
    struct tm tm;
    time_t timet = (time_t) MT_TIME_TO_SECONDS(t);
    unsigned int us;

    if (gmtime_r(&timet, &tm) == NULL) {
        return -1;
    }

    us = t % INT64_C(1000000);

    b->microsecond  = us % 1000;
    b->millisecond  = (us / 1000) % 1000;
    b->second       = tm.tm_sec;
    b->minute       = tm.tm_min;
    b->hour         = tm.tm_hour;
    b->date         = tm.tm_mday;
    b->month        = tm.tm_mon + 1;
    b->year         = tm.tm_year + 1900;
    b->week_day     = tm.tm_wday;

    return 0;
}

/* ------------------------------------------------------------------------- */

MT_Time MT_FromBrokenDownTime(MT_BrokenDownTime* b)
{
    struct tm tm;
    time_t t;
    MT_Time ret;

    tm.tm_sec   = b->second;
    tm.tm_min   = b->minute;
    tm.tm_hour  = b->hour;
    tm.tm_mday  = b->date;
    tm.tm_mon   = b->month - 1;
    tm.tm_year  = b->year - 1900;
    tm.tm_isdst = 0;

    t = timegm(&tm);

    if (t == -1) {
        return MT_TIME_INVALID;
    }

    ret = MT_TIME_FROM_SECONDS(t);
    ret += MT_TIME_FROM_MS(b->millisecond);
    ret += MT_TIME_FROM_US(b->microsecond);

    return ret;
}

#endif

