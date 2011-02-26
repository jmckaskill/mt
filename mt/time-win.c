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

#ifdef _WIN32
#include <windows.h>

/* -------------------------------------------------------------------------- */

MT_Time MT_FromFileTime(FILETIME* ft)
{
    uint64_t res = 0;
    res |= ft->dwHighDateTime;
    res <<= 32;
    res |= ft->dwLowDateTime;
    res /= 10; /* convert into microseconds */

    /* converting file Time to unix epoch */
    return (MT_Time)(res - INT64_C(11644473600000000));
}

/* -------------------------------------------------------------------------- */

void MT_ToFileTime(MT_Time t, FILETIME* ft)
{
    /* converting unix epoch to filetime epoch */
    uint64_t res = t + UINT64_C(11644473600000000);
    res *= 10; /* convert to 100ns increments */

    ft->dwHighDateTime = (uint32_t)(res >> 32);
    ft->dwLowDateTime  = (uint32_t) res;
}

/* -------------------------------------------------------------------------- */

#ifdef _WIN32_WCE
/* GetSystemTimeAsFileTime is not available on CE until CE6 */
MT_Time MT_CurrentTime()
{
    SYSTEMTIME st;
    FILETIME ft;
    GetSystemTime(&st);

    if (!SystemTimeToFileTime(&st, &ft)) {
        return MT_TIME_INVALID;
    }

    return MT_FromFileTime(&ft);
}

#else
MT_Time MT_CurrentTime()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return MT_FromFileTime(&ft);
}

#endif

/* -------------------------------------------------------------------------- */

int MT_ToBrokenDownTime(MT_Time t, MT_BrokenDownTime* tm)
{
    FILETIME ft;
    SYSTEMTIME st;

    MT_ToFileTime(t, &ft);

    if (!FileTimeToSystemTime(&ft, &st)) {
        return -1;
    }

    tm->week_day    = st.wDayOfWeek;
    tm->year        = st.wYear;
    tm->month       = st.wMonth;
    tm->date        = st.wDay;
    tm->hour        = st.wHour;
    tm->minute      = st.wMinute;
    tm->second      = st.wSecond;
    tm->millisecond = st.wMilliseconds;
    tm->microsecond = (int) (MT_TIME_TO_US(t) % 1000);

    return 0;
}

/* ------------------------------------------------------------------------- */

MT_Time MT_FromBrokenDownTime(MT_BrokenDownTime* tm)
{
    FILETIME ft;
    SYSTEMTIME st;
    MT_Time ret;

    st.wYear            = (WORD) tm->year;
    st.wMonth           = (WORD) tm->month;
    st.wDay             = (WORD) tm->date;
    st.wHour            = (WORD) tm->hour;
    st.wMinute          = (WORD) tm->minute;
    st.wSecond          = (WORD) tm->second;
    st.wMilliseconds    = (WORD) tm->millisecond;

    if (!SystemTimeToFileTime(&st, &ft)) {
        return MT_TIME_INVALID;
    }

    ret = MT_FromFileTime(&ft);

    if (!MT_TIME_ISVALID(ret)) {
        return ret;
    }

    return ret + MT_TIME_FROM_US(tm->microsecond);
}

#endif

