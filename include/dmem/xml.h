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

#include "char.h"
#include <delegate.h>

typedef struct dx_Node dx_Node;
typedef struct dx_Parser dx_Parser;
typedef struct dx_Attribute dx_Attribute;
typedef struct dx_Builder dx_Builder;

struct dx_Attribute {
    d_Slice(char) key;
    d_Slice(char) value;
};

DECLARE_DELEGATE_1(dx_Delegate, bool, dx_Node*);
DVECTOR_INIT(Attribute, dx_Attribute);

struct dx_Node {
    d_Slice(char)       value;
    d_Slice(Attribute)  attributes;
    dx_Delegate         on_element;
    dx_Delegate         on_inner_xml;
    dx_Delegate         on_end;
};

#define dx_Bind(func, obj) BIND1(dx_Delegate, func, obj, dx_Node**)

DMEM_API bool dx_parse(d_Slice(char) str, dx_Delegate dlg, d_Vector(char)* errstr);

DMEM_API dx_Parser* dx_new_parser(dx_Delegate dlg);
DMEM_API d_Slice(char) dx_parse_error(dx_Parser* p);
DMEM_API void dx_free_parser(dx_Parser* p);
DMEM_API int dx_parse_chunk(dx_Parser* p, d_Slice(char) str);

DMEM_API d_Slice(char) dx_attribute(const dx_Node* element, d_Slice(char) name);
DMEM_API bool dx_boolean_attribute(const dx_Node* element, d_Slice(char) name);
DMEM_API double dx_number_attribute(const dx_Node* element, d_Slice(char) name);

struct dx_Builder {
    d_Vector(char) out;
    bool in_element;
};

DMEM_API void dx_init_builder(dx_Builder* b);
DMEM_API void dx_clear_builder(dx_Builder* b);
DMEM_API void dx_destroy_builder(dx_Builder* b);

DMEM_API void dx_start_element(dx_Builder* b, d_Slice(char) tag);
DMEM_API void dx_end_element(dx_Builder* b, d_Slice(char) tag);

DMEM_API void dx_append_attribute(dx_Builder* b, d_Slice(char) key, d_Slice(char) value);
DMEM_API void dx_append_number_attribute(dx_Builder* b, d_Slice(char) key, double value);
DMEM_API void dx_append_boolean_attribute(dx_Builder* b, d_Slice(char) key, bool value);

DMEM_API void dx_append_xml(dx_Builder* b, d_Slice(char) text);
DMEM_API void dx_append_text(dx_Builder* b, d_Slice(char) text);

DMEM_API void dv_append_xml_encoded(d_Vector(char)* b, d_Slice(char) text);
DMEM_API void dv_append_xml_decoded(d_Vector(char)* b, d_Slice(char) text);

