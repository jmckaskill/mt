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

#define DMEM_LIBRARY

#include "xml.h"
#include <math.h>
#include <assert.h>

/* ------------------------------------------------------------------------- */

static void ThrowError(dx_Parser* p, const char* format, ...) DMEM_PRINTF(2, 3);

static void ThrowError(dx_Parser* p, const char* format, ...)
{
    if (p->errstr) {
        va_list ap;
        va_start(ap, format);
        dv_print(p->errstr, "(%d) : ", p->line_number);
        dv_vprint(p->errstr, format, ap);    
    }

    longjmp(p->jmp, DXI_ERROR);
}

/* ------------------------------------------------------------------------- */

#define UNICODE_MAX 0x10FFFF

static void AppendUtf8(d_Vector(char)* buffer, long ch)
{
    uint8_t out[4];

    if (ch < 0x7F) {
        /* 1 char */
        out[0] = (uint8_t) ch;
        dv_append2(buffer, out, 1);

    } else if (ch < 0x07FF) {
        /* 2 chars */
        out[0] = (uint8_t)(0xC0 | (ch >> 6));
        out[1] = (uint8_t)(0x80 | (ch & 0x3F));
        dv_append2(buffer, out, 2);

    } else if (ch < 0xFFFF) {
        /* 3 chars */
        out[0] = (uint8_t)(0xE0 | (ch >> 12));
        out[1] = (uint8_t)(0x80 | ((ch >> 6) & 0x3F));
        out[2] = (uint8_t)(0x80 | (ch & 0x3F));
        dv_append2(buffer, out, 3);

    } else {
        /* 4 chars */
        out[0] = (uint8_t)(0xF0 | (ch >> 18));
        out[1] = (uint8_t)(0x80 | ((ch >> 12) & 0x3F));
        out[2] = (uint8_t)(0x80 | ((ch >> 6) & 0x3F));
        out[3] = (uint8_t)(0x80 | (ch & 0x3F));
        dv_append2(buffer, out, 4);
    }
}

static bool IsWhitespace(char ch)
{
  return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

/* ------------------------------------------------------------------------- */

static bool ReplaceEscapedValue(d_Vector(char)* buffer, int begin)
{
    d_Slice(char) str = dv_right(*buffer, begin);

    if (dv_begins_with(str, C("&#x"))) {
        /* &#x33; */
        char* chend;
        long val = strtol(str.data + 3, &chend, 16);

        if (chend != str.data + str.size || val < 0 || val > UNICODE_MAX) {
            return false;
        }

        dv_resize(buffer, begin);
        AppendUtf8(buffer, val);
        return true;

    } else if (dv_begins_with(str, C("&#"))) {
        /* &#33; */
        char* chend;
        long val = strtol(str.data + 2, &chend, 10);

        if (chend != str.data + str.size || val < 0 || val > UNICODE_MAX) {
            return false;
        }

        dv_resize(buffer, begin);
        AppendUtf8(buffer, val);
        return true;

    } else if (dv_equals(str, C("&amp"))) {
        dv_resize(buffer, begin);
        dv_append(buffer, C("&"));
        return true;

    } else if (dv_equals(str, C("&quot"))) {
        dv_resize(buffer, begin);
        dv_append(buffer, C("\""));
        return true;

    } else if (dv_equals(str, C("&apos"))) {
        dv_resize(buffer, begin);
        dv_append(buffer, C("\'"));
        return true;

    } else if (dv_equals(str, C("&lt"))) {
        dv_resize(buffer, begin);
        dv_append(buffer, C("<"));
        return true;

    } else if (dv_equals(str, C("&gt"))) {
        dv_resize(buffer, begin);
        dv_append(buffer, C(">"));
        return true;

    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------- */

static d_Slice(char) GetToken(dx_Parser* s, const char** pb, const char* e)
{
    const char* b = *pb;
    const char* p = b;

    for (;;) {
        if (p == e) {
            dv_append2(&s->token_buffer, b, p - b);
            longjmp(s->jmp, DXI_NEED_MORE);

        } else if (IsWhitespace(*p) || *p == '=' || *p == '>' || *p == '?' || *p == '/' || *p == '[' || *p == ';') {

            *pb = p;

            if (s->token_buffer.size) {
                dv_append2(&s->token_buffer, b, p - b);
                return s->token_buffer;
            } else {
                return dv_char2(b, p - b);
            }

        } else {
            p++;
        }
    }
}

/* ------------------------------------------------------------------------- */

static d_Slice(char) FromOffset(dx_Parser* s, dxi_OffsetString str)
{
    if (str.data) {
        return dv_char2(str.data, str.size);
    } else {
        return dv_slice(s->open_buffer, str.off, str.size);
    }
}


static dxi_OffsetString TokenToOffset(dx_Parser* s, d_Slice(char) token)
{
    dxi_OffsetString ret;
    ret.size = token.size;

    if (token.data == s->token_buffer.data) {
        ret.off = s->open_buffer.size;
        ret.data = NULL;
        dv_append2(&s->open_buffer, token.data, token.size);
    } else {
        ret.data = token.data;
        ret.off = -1;
    }

    return ret;
}

/* ------------------------------------------------------------------------- */

static d_Slice(char) GetString(
        dx_Parser*      s,
        const char**    pb,
        const char*     e,
        char            quote)
{
    const char* b = *pb;
    const char* p = b;

    if (s->to_decode_begin >= 0) {
        p = memchr(p, ';', e - p);

        if (p == NULL) {
            dv_append2(&s->token_buffer, b, e - b);
            longjmp(s->jmp, DXI_NEED_MORE);
        }

        dv_append2(&s->token_buffer, b, p - b);

        if (!ReplaceEscapedValue(&s->token_buffer, s->to_decode_begin)) {
            ThrowError(s, "Invalid entity reference");
        }

        /* Skip over the ';' */
        p++;
        b = p;
        s->to_decode_begin = -1;
    }

    for (;;) {
        if (p == e) {
            dv_append2(&s->token_buffer, b, p - b);
            longjmp(s->jmp, DXI_NEED_MORE);

        } else if (*p == quote) {

            *pb = p + 1;

            if (s->token_buffer.size) {
                dv_append2(&s->token_buffer, b, p - b);
                return s->token_buffer;
            } else {
                return dv_char2(b, p - b);
            }

        } else if (*p == '&') {
            s->to_decode_begin = s->token_buffer.size;

            dv_append2(&s->token_buffer, b, p - b);
            b = p;

            p = memchr(p, ';', e - p);

            if (p == NULL) {
                dv_append2(&s->token_buffer, b, e - b);
                longjmp(s->jmp, DXI_NEED_MORE);
            }

            dv_append2(&s->token_buffer, b, p - b);

            if (!ReplaceEscapedValue(&s->token_buffer, s->to_decode_begin)) {
                ThrowError(s, "Invalid entity reference");
            }

            /* Skip over the ';' */
            p++;
            b = p;
            s->to_decode_begin = -1;

        } else if (*p == '<') {
            ThrowError(s, "< characters are invalid in attribute value strings");

        } else if (*p == '\n') {
            s->line_number++;
            p++;

        } else {
            p++;
        }
    }
}

/* ------------------------------------------------------------------------- */

static const char* ConsumeWhitespace(dx_Parser* p, const char* b, const char* e)
{
    for (;;) {
        if (b == e) {
            longjmp(p->jmp, DXI_NEED_MORE);
        } else if (*b == '\n') {
            p->line_number++;
            b++;
        } else if (IsWhitespace(*b)) {
            b++;
        } else {
            return b;
        }
    }
}

static const char* SearchForCharacter(dx_Parser* p, const char* b, const char* e, char ch)
{
    while (b < e) {
        if (*b == ch) {
            return b;
        } else if (*b == '\n') {
            p->line_number++;
            b++;
        } else {
            b++;
        }
    }

    longjmp(p->jmp, DXI_NEED_MORE);
}

/* ------------------------------------------------------------------------- */

static dxi_Scope* PushScope(dx_Parser* s, dx_Node* node, const char* b, d_Slice(char)* inner_xml)
{
    dxi_Scope* scope = dv_append_zeroed(&s->scopes, 1);
    scope->tag = s->current_tag;
    scope->tagbuf = s->tagbuf;
    scope->namespaces_size = s->namespaces.size;
    scope->on_element = node->on_element;
    scope->on_inner_xml = node->on_inner_xml;
    scope->on_end = node->on_end;
    memset(&s->current_tag, 0, sizeof(s->current_tag));
    memset(&s->tagbuf, 0, sizeof(s->tagbuf));

    if (node->on_inner_xml.func && s->outermost_inner_xml_scope < 0) {
        s->outermost_inner_xml_scope = s->scopes.size;
        *inner_xml = dv_right(*inner_xml, b - inner_xml->data);
        dv_clear(&s->inner_xml);
    }

    scope->inner_xml_off = s->inner_xml.size + (b - inner_xml->data);

    return scope;
}

static dxi_Scope* PopScope(dx_Parser* s)
{
    int i;
    dxi_Scope* scope = &dv_last(s->scopes);
    dv_free(scope->tagbuf);

    if (s->scopes.size == s->outermost_inner_xml_scope) {
        dv_clear(&s->inner_xml);
        s->outermost_inner_xml_scope = -1;
    }

    dv_erase_end(&s->scopes, 1);
    scope = &dv_last(s->scopes);

    for (i = scope->namespaces_size; i < s->namespaces.size; i++) {
        dv_free(s->namespaces.data[i]);
    }
    dv_resize(&s->namespaces, scope->namespaces_size);

    return scope;
}

static void SetupNode(dx_Parser* s, dxi_Scope* scope, dx_Node* node)
{
    d_Slice(char) tag;
    d_Slice(char) alias = DV_INIT;
    d_Slice(char) ns = DV_INIT;
    int i, j, colon;

    memset(node, 0, sizeof(dx_Node));


    /* Resolve the attributes */
    dv_resize(&s->attributes_out, s->attributes.size / 2);

    for (i = 0, j = 0; i < s->attributes.size; i += 2, j++) {
        dx_Attribute* a = &s->attributes_out.data[j];
        a->key = FromOffset(s, s->attributes.data[i]);
        a->value = FromOffset(s, s->attributes.data[i+1]);
    }

    node->attributes = s->attributes_out;


    /* Resolve the tag namespace */
    tag = s->current_tag;
    colon = dv_find_char(tag, ':');

    if (colon >= 0) {
        alias = dv_left(tag, colon);
        tag = dv_right(tag, colon + 1);
    } else {
        /* so that dv_right(str, colon + 1) is the entirity of str */
        assert(colon == -1);
    }

    for (i = s->namespaces.size - 2; i >= 0; i-=2) {
        if (dv_equals(alias, s->namespaces.data[i])) {
            ns = s->namespaces.data[i+1];
            break;
        }
    }
        
    if (ns.data == NULL && alias.size != 0) {
        ThrowError(s, "Could not resolve a namespace for the alias '%.*s'", DV_PRI(alias));
    }

    if (ns.size) {
        int off = s->open_buffer.size;
        dv_append(&s->open_buffer, ns);
        dv_append(&s->open_buffer, C(":"));
        dv_append(&s->open_buffer, tag);
        node->value = dv_right(s->open_buffer, off);
    } else {
        node->value = tag;
    }
}

int dx_parse_chunk(dx_Parser* s, d_Slice(char) str)
{
    const char* b = str.data;
    const char* e = b + str.size;
    dx_Node node;
    d_Slice(char) token, tag;
    d_Slice(char) inner_xml = str;
    dxi_Scope* scope = NULL;
    int ret;

    memset(&node, 0, sizeof(node));

    ret = setjmp(s->jmp);
    if (ret == DXI_ERROR) {
        return -1;
    } else if (ret == DXI_NEED_MORE) {
        int i;

        for (i = 0; i < s->scopes.size; i++) {
            dxi_Scope* x = &s->scopes.data[i];
            if (x->tag.data != x->tagbuf.data) {
                dv_set(&x->tagbuf, x->tag);
                x->tag = x->tagbuf;
            }
        }
        
        for (i = 0; i < s->attributes.size; i++) {
            dxi_OffsetString* o = &s->attributes.data[i];
            if (o->data) {
                o->off = s->open_buffer.size;
                dv_append2(&s->open_buffer, o->data, o->size);
                o->data = NULL;
            }
        }

        if (s->current_tag.data != s->tagbuf.data) {
            dv_set(&s->tagbuf, s->current_tag);
            s->current_tag = s->tagbuf;
        }
        
        if (s->outermost_inner_xml_scope >= 0) {
            dv_append(&s->inner_xml, inner_xml);
        }

        return str.size;
    }

    scope = &dv_last(s->scopes);

    switch (s->state) {

        /* Searching for < to start the next element */
    next_element:
    case DXI_NEXT_ELEMENT:
        s->state = DXI_NEXT_ELEMENT;
        b = SearchForCharacter(s, b, e, '<');
        b++;
        goto tag_type;

        /* Have < switching between </, <tag, <?, <![CDATA[, and <!-- */
    tag_type:
    case DXI_TAG_TYPE:
        s->state = DXI_TAG_TYPE;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);

        } else if (*b == '/') {
            if (s->scopes.size == 1) {
                ThrowError(s, "Close tag at root scope");
            }

            b++;
            goto close_tag;

        } else if (*b == '?') {
            b++;
            goto processing_instruction;

        } else if (*b == '!') {
            b++;
            goto bang_tag;

        } else if (!IsWhitespace(*b)) {
            goto open_tag;

        } else {
            ThrowError(s, "Whitespace is not allowed immediately after opening a tag");
        }

        /* Seaching for ? in ?> */
    processing_instruction:
    case DXI_PROCESSING_INSTRUCTION:
        s->state = DXI_PROCESSING_INSTRUCTION;
        b = SearchForCharacter(s, b, e, '?');
        b++;
        goto processing_instruction_2;

        /* Seaching for > in ?> */
    processing_instruction_2:
    case DXI_PROCESSING_INSTRUCTION_2:
        s->state = DXI_PROCESSING_INSTRUCTION_2;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);
        } else if (*b == '>') {
            b++;
            goto next_element;
        } else {
            goto processing_instruction;
        }

        /* Searching for first [ in <![CDATA[ or first - in <!-- */
    bang_tag:
    case DXI_BANG_TAG:
        s->state = DXI_BANG_TAG;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);
        } else if (*b == '[') {
            b++;
            goto cdata;
        } else if (*b == '-') {
            b++;
            goto comment_begin;
        } else {
            ThrowError(s, "Invalid character in tag after \"<!\"");
        }

        /* Searching for CDATA in <![CDATA[ */
    cdata:
        dv_clear(&s->token_buffer);
    case DXI_CDATA:
        s->state = DXI_CDATA;
        token = GetToken(s, &b, e);
        if (!dv_equals(token, C("CDATA"))) {
            ThrowError(s, "Invalid data after \"<![\" - expected CDATA");
        }
        goto cdata_begin;

        /* Searching for last [ in <![CDATA[ */
    cdata_begin:
    case DXI_CDATA_BEGIN:
        s->state = DXI_CDATA_BEGIN;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);
        } else if (*b == '[') {
            b++;
            goto cdata_end;
        } else {
            ThrowError(s, "Invalid character after \"<![CDATA\" - expected [");
        }

        /* Searching for first ] in ]]> */
    cdata_end:
    case DXI_CDATA_END:
        s->state = DXI_CDATA_END;
        b = SearchForCharacter(s, b, e, ']');
        b++;
        goto cdata_end_2;

        /* Searching for second ] in ]]> */
    cdata_end_2:
    case DXI_CDATA_END_2:
        s->state = DXI_CDATA_END_2;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);
        } else if (*b == ']') {
            b++;
            goto cdata_end_3;
        } else {
            goto cdata_end;
        }

        /* Searching for > in ]]> */
    cdata_end_3:
    case DXI_CDATA_END_3:
        s->state = DXI_CDATA_END_3;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);
        } else if (*b == '>') {
            b++;
            goto next_element;
        } else {
            goto cdata_end;
        }

        /* Searching for second - in <!-- */
    comment_begin:
    case DXI_COMMENT_BEGIN:
        s->state = DXI_COMMENT_BEGIN;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);
        } else if (*b == '-') {
            b++;
            goto comment_end;
        } else {
            ThrowError(s, "Invalid character after \"<!-\" - expected -");
        }

        /* Searching for first - in --> */
    comment_end:
    case DXI_COMMENT_END:
        s->state = DXI_COMMENT_END;
        b = SearchForCharacter(s, b, e, '-');
        b++;
        goto comment_end_2;

        /* Searching for second - in --> */
    comment_end_2:
    case DXI_COMMENT_END_2:
        s->state = DXI_COMMENT_END_2;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);
        } else if (*b == '-') {
            b++;
            goto comment_end_3;
        } else {
            goto comment_end;
        }

        /* Searching for > in --> */
    comment_end_3:
    case DXI_COMMENT_END_3:
        s->state = DXI_COMMENT_END_3;
        if (b == e) {
            longjmp(s->jmp, DXI_NEED_MORE);
        } else if (*b == '>') {
            b++;
            goto next_element;
        } else {
            ThrowError(s, "The string '--' is not allowed within xml comments");
        }

        /* We have < and know this is followed by a tag name, this gets the
         * tag name out.
         */
    open_tag:
        dv_clear(&s->token_buffer);
    case DXI_OPEN_TAG:
        s->state = DXI_OPEN_TAG;
        token = GetToken(s, &b, e);

        if (token.data == s->token_buffer.data) {
            dv_set(&s->tagbuf, token);
            s->current_tag = s->tagbuf;
        } else {
            s->current_tag = token;
        }

        goto next_attribute;

        /* In an open element, looking for the next key="value", > or /> */
    next_attribute:
    case DXI_NEXT_ATTRIBUTE:
        s->state = DXI_NEXT_ATTRIBUTE;
        b = ConsumeWhitespace(s, b, e);

        if (*b == '>') {
            b++;

            if (scope->on_element.func) {
                SetupNode(s, scope, &node);
                if (scope->on_element.func && !CALL_DELEGATE_1(scope->on_element, &node)) {
                    ThrowError(s, "Callback abort");
                }
                dv_clear(&s->attributes);
                dv_clear(&s->open_buffer);
            }

            scope = PushScope(s, &node, b, &inner_xml);
            goto next_element;

        } else if (*b == '/') {
            b++;

            if (scope->on_element.func) {
                SetupNode(s, scope, &node);
                if (scope->on_element.func && !CALL_DELEGATE_1(scope->on_element, &node)) {
                    ThrowError(s, "Callback abort");
                }
                dv_clear(&s->attributes);
                dv_clear(&s->open_buffer);
            }

            scope = PushScope(s, &node, b, &inner_xml);
            goto finish_close_element;

        } else {
            goto attribute_key;
        }

        /* In an open element at the start of a key="value" attribute */
    attribute_key:
        dv_clear(&s->token_buffer);
    case DXI_ATTRIBUTE_KEY:
        s->state = DXI_ATTRIBUTE_KEY;
        token = GetToken(s, &b, e);

        if (dv_equals(token, C("xmlns"))) {
            dv_append_zeroed(&s->namespaces, 1);
            s->in_namespace_attribute = true;

        } else if (dv_begins_with(token, C("xmlns:"))) {
            d_Vector(char) ns = DV_INIT;
            dv_set(&ns, dv_right(token, C("xmlns:").size));
            dv_append1(&s->namespaces, ns);
            s->in_namespace_attribute = true;

        } else {
            dv_append1(&s->attributes, TokenToOffset(s, token));
            s->in_namespace_attribute = false;
        }

        goto attribute_equals;

        /* In an attribute between the key and the = */
    attribute_equals:
    case DXI_ATTRIBUTE_EQUALS:
        s->state = DXI_ATTRIBUTE_EQUALS;
        b = ConsumeWhitespace(s, b, e);
        if (*b == '=') {
            b++;
            goto attribute_quote;
        } else {
            ThrowError(s, "Invalid character between attribute key and value - expected =");
        }

        /* In an attribute between the = and the start of the quoted value */
    attribute_quote:
    case DXI_ATTRIBUTE_QUOTE:
        s->state = DXI_ATTRIBUTE_QUOTE;
        b = ConsumeWhitespace(s, b, e);

        if (*b == '\'') {
            b++;
            goto attribute_value_single;

        } else if (*b == '\"') {
            b++;
            goto attribute_value_double;

        } else {
            ThrowError(s, "Invalid character starting an attribute value - expected \" or \'");
        }

        /* In a single quoted attribute value */
    attribute_value_single:
        dv_clear(&s->token_buffer);
    case DXI_ATTRIBUTE_VALUE_SINGLE:
        s->state = DXI_ATTRIBUTE_VALUE_SINGLE;
        token = GetString(s, &b, e, '\'');

        if (s->in_namespace_attribute) {
            d_Vector(char) ns = DV_INIT;
            dv_set(&ns, token);
            dv_append1(&s->namespaces, ns);
        } else {
            dv_append1(&s->attributes, TokenToOffset(s, token));
        }

        goto next_attribute;

        /* In a double quoted attribute value */
    attribute_value_double:
        dv_clear(&s->token_buffer);
    case DXI_ATTRIBUTE_VALUE_DOUBLE:
        s->state = DXI_ATTRIBUTE_VALUE_DOUBLE;
        token = GetString(s, &b, e, '\"');

        if (s->in_namespace_attribute) {
            d_Vector(char) ns = DV_INIT;
            dv_set(&ns, token);
            dv_append1(&s->namespaces, ns);
        } else {
            dv_append1(&s->attributes, TokenToOffset(s, token));
        }

        goto next_attribute;

        /* Have </ need to get and check the tag against the open element */
    close_tag:
        dv_clear(&s->token_buffer);
    case DXI_CLOSE_TAG:
        s->state = DXI_CLOSE_TAG;
        token = GetToken(s, &b, e);

        if (!dv_equals(token, scope->tag)) {
            ThrowError(s, "Mismatch between close and open tags - open is '%.*s', close is '%.*s'", DV_PRI(tag), DV_PRI(token));
        }

        goto finish_close_element;

        /* In a close element after the tag or after the / in an instant close
         * element.
         */
    finish_close_element:
    case DXI_FINISH_CLOSE_ELEMENT:
        s->state = DXI_FINISH_CLOSE_ELEMENT;
        b = ConsumeWhitespace(s, b, e);
        if (*b != '>') {
            ThrowError(s, "Invalid character finishing an element - expected >");
        }

        memset(&node, 0, sizeof(node));

        if (scope->on_inner_xml.func) {
            int in_buf = s->inner_xml.size - scope->inner_xml_off;
            int b_off = b - inner_xml.data;

            if (in_buf >= 0) {
                /* Part of our inner_xml is in s->inner_xml so we need to copy
                 * what we need to s->inner_xml and use the copy in there.
                 */
                dv_append(&s->inner_xml, dv_left(inner_xml, b_off));
                node.value = dv_right(s->inner_xml, scope->inner_xml_off);
                inner_xml = dv_right(inner_xml, b_off);
            } else {
                /* None of our inner_xml is in s->inner_xml, but
                 * the beginning of our text may be some way through
                 * inner_xml.
                 */
                int begin = -in_buf;
                node.value = dv_slice(inner_xml, begin, b_off - begin);
            }

            /* the inner text has the closing tag '</foo   ' at the end which
             * we need to remove
             */

            while (node.value.size > 0 && IsWhitespace(dv_last(node.value))) {
                node.value.size--;
            }

            if (node.value.size >= strlen("</") + scope->tag.size) {
                node.value.size -= strlen("</") + scope->tag.size;
            }

            if (!CALL_DELEGATE_1(scope->on_inner_xml, &node)) {
                ThrowError(s, "Callback abort");
            }

            memset(&node, 0, sizeof(node));
        }

        if (scope->on_end.func && !CALL_DELEGATE_1(scope->on_end, &node)) {
            ThrowError(s, "Callback abort");
        }

        scope = PopScope(s);
        goto next_element;
    }

    /* Shouldn't get here */
    abort();
    return -1;
}

/* ------------------------------------------------------------------------- */

dx_Parser* dx_new_parser(dx_Delegate dlg)
{
    dx_Parser* s = NEW(dx_Parser);

    dxi_Scope* scope = dv_append_zeroed(&s->scopes, 1);
    scope->on_element = dlg;

    s->errstr = &s->error_buffer;
    s->line_number = 1;
    s->state = DXI_NEXT_ELEMENT;
    s->to_decode_begin = -1;
    s->outermost_inner_xml_scope = -1;

    return s;
}

/* ------------------------------------------------------------------------- */

void dx_free_parser(dx_Parser* s)
{
    if (s) {
        int i;

        for (i = 0; i < s->scopes.size; i++) {
            dv_free(s->scopes.data[i].tagbuf);
        }
        dv_free(s->scopes);

        dv_clear_string_vector(&s->namespaces);
        dv_free(s->namespaces);

        dv_free(s->error_buffer);
        dv_free(s->open_buffer);
        dv_free(s->tagbuf);
        dv_free(s->attributes_out);
        dv_free(s->attributes);
        dv_free(s->token_buffer);
        dv_free(s->inner_xml);
        free(s);
    }
}

/* ------------------------------------------------------------------------- */

d_Slice(char) dx_parse_error(dx_Parser* s)
{
    return s->error_buffer;
}





/* ------------------------------------------------------------------------- */

bool dx_parse(d_Slice(char) str, dx_Delegate dlg, d_Vector(char)* errstr)
{
    int ret;
    dx_Parser* s = dx_new_parser(dlg);
    s->errstr = errstr;
    ret = dx_parse_chunk(s, str);
    dx_free_parser(s);
    return ret;
}

/* ------------------------------------------------------------------------- */

d_Slice(char) dx_attribute(const dx_Node* element, d_Slice(char) name)
{
    int i;
    for (i = 0; i < element->attributes.size; i++) {
        const dx_Attribute* a = &element->attributes.data[i];
        if (dv_equals(a->key, name)) {
            return a->value;
        }
    }

    return dv_char2(NULL, 0);
}

/* ------------------------------------------------------------------------- */

bool dx_boolean_attribute(const dx_Node* element, d_Slice(char) name)
{ return dv_to_boolean(dx_attribute(element, name)); }

double dx_number_attribute(const dx_Node* element, d_Slice(char) name)
{ return dv_to_number(dx_attribute(element, name)); }




/* ------------------------------------------------------------------------- */

void dv_append_xml_decoded(d_Vector(char)* out, d_Slice(char) text)
{
    const char* b = text.data;
    const char* p = b;
    const char* e = b + text.size;

    while (p < e) {
        p = memchr(p, '&', e - p);
        if (p) {
            int off = out->size + p - b;

            while (p < e && *p != ';') {
                p++;
            }

            if (p == e) break;

            dv_append2(out, b, p - b);
            if (!ReplaceEscapedValue(out, off)) {
                dv_append(out, C(";"));
            }

            p++;
            b = p;
        } else {
            p = e;
            break;
        }
    }

    dv_append2(out, b, p - b);
}

/* ------------------------------------------------------------------------- */

void dv_append_xml_encoded(d_Vector(char)* out, d_Slice(char) text)
{
    const char* b = text.data;
    const char* p = b;
    const char* e = b + text.size;

    while (p < e) {
        if (*p == '&') {
            dv_append2(out, b, p - b);
            dv_append(out, C("&apos;"));
            p = b = p + 1;

        } else if (*p == '<') {
            dv_append2(out, b, p - b);
            dv_append(out, C("&lt;"));
            p = b = p + 1;

        } else if (*p == '>') {
            dv_append2(out, b, p - b);
            dv_append(out, C("&gt;"));
            p = b = p + 1;

        } else if (*p == '\"') {
            dv_append2(out, b, p - b);
            dv_append(out, C("&quot;"));
            p = b = p + 1;

        } else if (*p == '\'') {
            dv_append2(out, b, p - b);
            dv_append(out, C("&apos;"));
            p = b = p + 1;

        } else {
            p++;
        }
    }

    dv_append2(out, b, p - b);
}

/* ------------------------------------------------------------------------- */

void dx_init_builder(dx_Builder* b)
{
    memset(b, 0, sizeof(dx_Builder));
    b->in_element = false;
}

/* ------------------------------------------------------------------------- */

void dx_clear_builder(dx_Builder* b)
{
    dv_clear(&b->out);
    b->in_element = false;
}

/* ------------------------------------------------------------------------- */

void dx_destroy_builder(dx_Builder* b)
{
    dv_free(b->out);
}

/* ------------------------------------------------------------------------- */

void dx_start_element(dx_Builder* b, d_Slice(char) tag)
{
    if (b->in_element) {
        dv_append(&b->out, C(">"));
    }

    dv_append(&b->out, C("<"));
    dv_append(&b->out, tag);
    b->in_element = true;
}

/* ------------------------------------------------------------------------- */

void dx_end_element(dx_Builder* b, d_Slice(char) tag)
{
    if (b->in_element) {
        dv_append(&b->out, C("/>"));
    } else {
        dv_append(&b->out, C("</"));
        dv_append(&b->out, tag);
        dv_append(&b->out, C(">"));
    }

    b->in_element = false;
}

/* ------------------------------------------------------------------------- */

void dx_append_attribute(dx_Builder* b, d_Slice(char) key, d_Slice(char) value)
{
    assert(b->in_element);
    dv_append(&b->out, C(" "));
    dv_append(&b->out, key);
    dv_append(&b->out, C("=\""));
    dv_append_xml_encoded(&b->out, value);
    dv_append(&b->out, C("\""));
}

/* ------------------------------------------------------------------------- */

void dx_append_number_attribute(dx_Builder* b, d_Slice(char) key, double value)
{
    assert(b->in_element);
    dv_append(&b->out, C(" "));
    dv_append(&b->out, key);
    dv_append(&b->out, C("=\""));
    dv_print(&b->out, "%.16g", value);
    dv_append(&b->out, C("\""));
}

/* ------------------------------------------------------------------------- */

void dx_append_boolean_attribute(dx_Builder* b, d_Slice(char) key, bool value)
{
    assert(b->in_element);
    dv_append(&b->out, C(" "));
    dv_append(&b->out, key);
    dv_append(&b->out, C("=\""));
    dv_append(&b->out, value ? C("true") : C("false"));
    dv_append(&b->out, C("\""));
}

/* ------------------------------------------------------------------------- */

void dx_append_xml(dx_Builder* b, d_Slice(char) text)
{
    if (b->in_element) {
        dv_append(&b->out, C(">"));
    }

    dv_append(&b->out, text);
    b->in_element = false;
}

/* ------------------------------------------------------------------------- */

void dx_append_text(dx_Builder* b, d_Slice(char) text)
{
    if (b->in_element) {
        dv_append(&b->out, C(">"));
    }

    dv_append_xml_encoded(&b->out, text);
    b->in_element = false;
}

