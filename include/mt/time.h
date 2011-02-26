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
#include <dmem/char.h>

/* MT_Time gives the time in microseconds centered on the unix epoch (midnight
 * Jan 1 1970) */

#define MT_TIME_INVALID           INT64_MAX
#define MT_TIME_ISVALID(x)        ((x) != MT_TIME_INVALID)

#define MT_TIME_DELTA             (1.0 / 1000000.0)
#define MT_TIME_FROM_US(x)        ((MT_Time) (x))
#define MT_TIME_FROM_MS(x)        ((MT_Time) ((double) (x) * 1000))
#define MT_TIME_FROM_SECONDS(x)   ((MT_Time) ((double) (x) * 1000000))
#define MT_TIME_FROM_MINUTES(x)   ((MT_Time) ((double) (x) * 1000000 * 60))
#define MT_TIME_FROM_HOURS(x)     ((MT_Time) ((double) (x) * 1000000 * 3600))
#define MT_TIME_FROM_DAYS(x)      ((MT_Time) ((double) (x) * 1000000 * 3600 * 24))
#define MT_TIME_FROM_WEEKS(x)     ((MT_Time) ((double) (x) * 1000000 * 3600 * 24 * 7))
#define MT_TIME_FROM_HZ(x)        ((MT_Time) ((1.0 / (double) (x)) * 1000000))

#define MT_TIME_TO_US(x)          (x)
#define MT_TIME_TO_MS(x)          ((double) (x) / 1000)
#define MT_TIME_TO_SECONDS(x)     ((double) (x) / 1000000)
#define MT_TIME_TO_HOURS(x)       ((double) (x) / 1000000 / 3600)
#define MT_TIME_TO_DAYS(x)        ((double) (x) / 1000000 / 3600 / 24 / 7)
#define MT_TIME_TO_WEEKS(x)       ((double) (x) / 1000000 / 3600 / 24 / 7)

#define MT_TIME_GPS_EPOCH         MT_TIME_FROM_SECONDS(315964800)


struct MT_BrokenDownTime {
    int microsecond;    /* Range [0,999] */
    int millisecond;    /* Range [0,999] */
    int second;         /* Range [0,60] - can be 60 to allow for leap seconds */
    int minute;         /* Range [0,59] */
    int hour;           /* Range [0,23] */
    int date;           /* Range [1,31] */
    int month;          /* Range [1,12] */
    int year;           /* Years according to gregorian calendar */
    int week_day;       /* Range [0-6] - number of days since sunday */
};

/* For converting from broken down time the week_day field is unused */

MT_API MT_Time MT_FromBrokenDownTime(MT_BrokenDownTime* tm); /* Returns MT_TIME_INVALID on error */
MT_API int MT_ToBrokenDownTime(MT_Time t, MT_BrokenDownTime* tm); /* Returns non zero on error */

#ifdef _WIN32
typedef struct _FILETIME FILETIME;
MT_API MT_Time MT_FromFileTime(FILETIME* ft);
MT_API void    MT_ToFileTime(MT_Time, FILETIME* ft);
#endif

/* Current time in UTC */
MT_API MT_Time MT_CurrentTime();

/* These return ISO 8601 strings eg "2010-02-16" and "2010-02-16T22:00:08.067890Z" */
MT_API void MT_DateString(d_Vector(char)* out, MT_Time t);
MT_API void MT_TimeString(d_Vector(char)* out, MT_Time t);
MT_API void MT_DateTimeString(d_Vector(char)* out, MT_Time t);
MT_API void MT_FormatTimeString(d_Vector(char)* out, const char* fmt, MT_Time t);


