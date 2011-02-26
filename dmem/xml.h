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

#include <dmem/xml.h>
#include <setjmp.h>


enum dxi_ParseState {
    DXI_NEXT_ELEMENT,
    DXI_TAG_TYPE,
    DXI_PROCESSING_INSTRUCTION,
    DXI_PROCESSING_INSTRUCTION_2,
    DXI_BANG_TAG,
    DXI_CDATA,
    DXI_CDATA_BEGIN,
    DXI_CDATA_END,
    DXI_CDATA_END_2,
    DXI_CDATA_END_3,
    DXI_COMMENT_BEGIN,
    DXI_COMMENT_END,
    DXI_COMMENT_END_2,
    DXI_COMMENT_END_3,
    DXI_OPEN_TAG,
    DXI_NEXT_ATTRIBUTE,
    DXI_ATTRIBUTE_KEY,
    DXI_ATTRIBUTE_EQUALS,
    DXI_ATTRIBUTE_QUOTE,
    DXI_ATTRIBUTE_VALUE_SINGLE,
    DXI_ATTRIBUTE_VALUE_DOUBLE,
    DXI_CLOSE_TAG,
    DXI_FINISH_CLOSE_ELEMENT
};

typedef enum dxi_ParseState dxi_ParseState;
typedef struct dxi_Scope dxi_Scope;
typedef struct dxi_OffsetString dxi_OffsetString;

#define DXI_ERROR     1
#define DXI_NEED_MORE 2

struct dxi_OffsetString {
    const char* data;
    int off;
    int size;
};

struct dxi_Scope {
    dx_Delegate             on_element;
    dx_Delegate             on_inner_xml;
    dx_Delegate             on_end;
    d_Slice(char)           tag;
    d_Vector(char)          tagbuf;
    int                     namespaces_size;
    int                     inner_xml_off;
};

DVECTOR_INIT(OffsetString, dxi_OffsetString);
DVECTOR_INIT(Scope, dxi_Scope);

struct dx_Parser {
    dxi_ParseState          state;
    int                     line_number;
    jmp_buf                 jmp;
    d_Vector(char)*         errstr;
    d_Vector(char)          error_buffer;
    d_Vector(Scope)         scopes;

    d_Vector(char_vector)   namespaces;

    d_Vector(char)          open_buffer;
    d_Vector(Attribute)     attributes_out;
    d_Vector(OffsetString)  attributes;

    d_Slice(char)           current_tag;
    d_Vector(char)          tagbuf;

    bool                    in_namespace_attribute;

    d_Vector(char)          token_buffer;
    int                     to_decode_begin;

    d_Vector(char)          inner_xml;
    int                     outermost_inner_xml_scope;
};

