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
#include "char.h"
#include "wchar.h"

/* ------------------------------------------------------------------------- */

DMEM_API int dv_append_file(d_Vector(char)* v, d_Slice(char) file);


DMEM_API int dv_append_file_win32(d_Vector(char)* v, d_Slice(wchar) folder, d_Slice(char) file);
DMEM_API int dv_append_file_posix(d_Vector(char)* v, d_Slice(char) folder, d_Slice(char) file);
DMEM_API int dv_append_file_openat(d_Vector(char)* v, int dfd, d_Slice(char) file);

#if _XOPEN_SOURCE + 0 >= 700 || _POSIX_C_SOURCE + 0 >= 200809L
#define DMEM_HAVE_OPENAT
#endif
