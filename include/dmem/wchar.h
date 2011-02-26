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

#pragma once

#include "common.h"
#include "vector.h"
#include "char.h"
#include <stdint.h>
#include <wchar.h>

/* Note that d_Slice(wchar) is a slice of uint16_t not wchar_t. Unfortunately
 * wchar_t is defined as uint32_t for UTF32 on some systems. Instead we use
 * uint16_t to force a d_Slice(wchar) to hold UTF16 (and be compatible with
 * wchar_t on windows where it is most useful).
 */
DVECTOR_INIT(wchar, uint16_t);

/* ------------------------------------------------------------------------- */

#if WCHAR_MAX == UINT16_MAX

/* Macro to convert wchar string literals to a d_Slice(wchar) */
#define W(STR) dv_wchar2(L##STR, sizeof(STR) - 1)

/* Convert a null-terminated wchar_t string 'wstr' to a d_Slice(wchar) - only
 * available on systems with sizeof(wchar_t) == sizeof(uint16_t).
 */
DMEM_INLINE d_Slice(wchar) dv_wchar(const wchar_t* wstr)
{
    d_Vector(wchar) ret;
    ret.size = wstr ? (int) wcslen(wstr) : 0;
    ret.data = (uint16_t*) wstr;
    return ret;
}

/* Convert a wchar_t string 'wstr' of length 'sz' to a d_Slice(wchar) - only
 * available on systems with sizeof(wchar_t) == sizeof(uint16_t).
 */
DMEM_INLINE d_Slice(wchar) dv_wchar2(const wchar_t* wstr, size_t sz)
{
    d_Vector(wchar) ret;
    ret.size = (int) sz;
    ret.data = (uint16_t*) wstr;
    return ret;
}

#endif

/* ------------------------------------------------------------------------- */

/* Convert a UTF16 string 'wstr' of length 'sz' to a d_Slice(wchar) */
DMEM_INLINE d_Slice(wchar) dv_utf16(const uint16_t* wstr, size_t sz)
{
    d_Vector(wchar) ret;
    ret.size = (int) sz;
    ret.data = (uint16_t*) wstr;
    return ret;
}

/* ------------------------------------------------------------------------- */

/* Appends a UTF8 encoded version of the UTF16 in 'from' to 'str'. */
DMEM_API void dv_append_from_utf16(d_Vector(char)* str, d_Slice(wchar) from);

/* Appends a UTF16 encoded version of the UTF8 in 'from' to 'wstr'. */
DMEM_API void dv_append_from_utf8(d_Vector(wchar)* wstr, d_Slice(char) from);

/* ------------------------------------------------------------------------- */

/* Requires dn >= 3 * (sn + 1) */
DMEM_API int utf16_to_utf8(uint8_t* dest, int dn, const uint16_t* src, int sn);

/* Requires dn >= sn */
DMEM_API int utf8_to_utf16(uint16_t* dest, int dn, const uint8_t* src, int sn);
