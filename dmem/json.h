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
#define DMEM_LIBRARY
#include <dmem/json.h>
#include <setjmp.h>

enum dji_ParseState {
    DJI_VALUE_BEGIN,
    DJI_NEXT,
    DJI_OBJECT_NEXT,
    DJI_KEY_STRING,
    DJI_OBJECT_COLON,
    DJI_VALUE_STRING,
    DJI_VALUE_NUMBER,
    DJI_VALUE_TOKEN,
    DJI_UTF8_BOM_2,
    DJI_UTF8_BOM_3
};

#define DJI_ERROR       1
#define DJI_NEED_MORE   2

typedef enum dji_ParseState dji_ParseState;
typedef struct dji_Scope dji_Scope;
typedef struct dji_Lexer dji_Lexer;

struct dji_Scope {
    dj_Delegate dlg;
    dj_NodeType type;
};

DVECTOR_INIT(Scope, dji_Scope);

struct dji_Lexer {
    d_Vector(char)      buf;
    d_Vector(char)      partial;
};

struct dj_Parser {
    dji_Lexer           key;
    dji_Lexer           value;
    d_Vector(Scope)     scopes;
    dji_ParseState      state;
    d_Slice(char)       current_key;
    d_Vector(char)*     errstr;
    d_Vector(char)      error_string_buffer;
    int                 line_number;
    jmp_buf             jmp;
};


