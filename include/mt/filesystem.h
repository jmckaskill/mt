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
#include <dmem/char.h>

typedef struct MT_DirEntry MT_DirEntry;

struct MT_DirEntry {
    d_Slice(char) name;
    bool          is_folder;
    bool          is_hidden;
    uint64_t      size;
    MT_Time       modify_time;
};

/* ------------------------------------------------------------------------- */

MT_API MT_Directory* MT_OpenDirectory(d_Slice(char) folder);
MT_API MT_Directory* MT_OpenDirectory2(MT_Directory* dir, d_Slice(char) relative);
MT_API void MT_CloseDirectory(MT_Directory* dir);
MT_API bool MT_HaveNextDirEntry(MT_Directory* dir, MT_DirEntry* dirent);
MT_API int MT_AppendFile(d_Vector(char)* out, MT_Directory* dir, d_Slice(char) name);
MT_API bool MT_IsDirectory(d_Slice(char) name);

