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
#include <delegate.h>

enum dj_NodeType {
    DJ_END,
    DJ_STRING,
    DJ_NUMBER,
    DJ_BOOLEAN,
    DJ_ARRAY,
    DJ_OBJECT,
    DJ_NULL
};

typedef enum dj_NodeType dj_NodeType;
typedef struct dj_Node dj_Node;
typedef struct dj_Parser dj_Parser;
typedef struct dj_Builder dj_Builder;

DECLARE_DELEGATE_1(dj_Delegate, bool, dj_Node*);

struct dj_Node {
    d_Slice(char)   key;

    dj_NodeType     type;
    d_Slice(char)   string;
    bool            boolean;
    double          number;

    dj_Delegate     on_child;
};

#define dj_Bind(func, obj) BIND1(dj_Delegate, func, obj, dj_Node**)

DMEM_API int dj_parse(d_Slice(char) str, dj_Delegate dlg, d_Vector(char)* errstr);

DMEM_API dj_Parser* dj_new_parser(dj_Delegate dlg);
DMEM_API int dj_parse_chunk(dj_Parser* p, d_Slice(char) str);
DMEM_API int dj_parse_complete(dj_Parser* p);
DMEM_API d_Slice(char) dj_parse_error(dj_Parser* p);
DMEM_API void dj_free_parser(dj_Parser* p);

struct dj_Builder {
    d_Vector(char) out;
    int depth;
    bool just_started_object;
    bool have_key;
};

DMEM_API void dj_init_builder(dj_Builder* b);
DMEM_API void dj_destroy_builder(dj_Builder* b);

DMEM_API void dj_start_object(dj_Builder* b);
DMEM_API void dj_end_object(dj_Builder* b);
DMEM_API void dj_start_array(dj_Builder* b);
DMEM_API void dj_end_array(dj_Builder* b);

DMEM_API void dj_append_key(dj_Builder* b, d_Slice(char) key);
DMEM_API void dj_append_string(dj_Builder* b, d_Slice(char) value);
DMEM_API void dj_append_number(dj_Builder* b, double value);
DMEM_API void dj_append_boolean(dj_Builder* b, bool value);
DMEM_API void dj_append_null(dj_Builder* b);

