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
#include "json.h"
#include <dmem/wchar.h>
#include <math.h>

/* -------------------------------------------------------------------------- */

static void ThrowError(dj_Parser* p, const char* format, ...)
{
  if (p->errstr) {
      va_list ap;
      dv_print(p->errstr, "(%d) : ", p->line_number);
      va_start(ap, format);
      dv_vprint(p->errstr, format, ap);
  }

  longjmp(p->jmp, DJI_ERROR);
}

/* -------------------------------------------------------------------------- */

static bool IsHex(char ch)
{
    return ('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F');
}

static bool IsControlChar(char ch)
{
    return '\0' <= ch && ch < ' ';
}

static const char* GetString(dj_Parser* parser, dji_Lexer* s, const char* b, const char* e, d_Slice(char)* out)
{
    const char* p = b;
    const char* ret = NULL;

    /* Find the end of the string append it to s->partial and then decode
     * s->partial into s->buf.
     */
    if (s->partial.size) {

        /* The end of the string is the first " which is not escaped. In order
         * to figure out if a " is escaped we must run through the string
         * processing the escapes. There is a corner case where the escaping
         * slash is split from the ", in which case we rely on the fact that
         * the main parse will remove everything but the \ from the partial
         * buffer.
         */

        /* Skip over a partial \ which is escaping a " */
        if (s->partial.size == 1 && s->partial.data[0] == '\\' && e > p) {
            p++;
        }

        for (;;) {
            if (p == e) {
                /* Parse whatever we can of s->partial */
                dv_append2(&s->partial, b, p - b);
                break;

            } else if (*p == '\"') {
                p++; /* include the " in s->partial */
                dv_append2(&s->partial, b, p - b);
                break;

            } else if (*p == '\\') {
                p++;

                if (p == e) {
                    /* Parse whatever we can of s->partial */
                    dv_append2(&s->partial, b, p - b);
                    break;
                } else {
                    /* Skip over the escaped character */
                    p++;
                }

            } else {
                p++;
            }
        }

        ret = p;

        b = s->partial.data;
        e = b + s->partial.size;
        p = b;
    }

    for (;;) {

        if (p == e) {
            goto out_of_data;

        } else if (*p == '\"') {

            if (!ret) {
                ret = p + 1;
            }

            if (s->buf.size) {
                dv_append2(&s->buf, b, p - b);
                *out = s->buf;
            } else {
                *out = dv_char2(b, p - b);
            }

            return ret;

        } else if (IsControlChar(*p)) {
            ThrowError(parser, "Control characters are not allowed directly within strings. "
                            "Use the \\[bfntr] or \\u escape mechanisms instead");

        } else if (*p == '\\') {

            dv_append2(&s->buf, b, p - b); 
            b = p;

            if (p + 2 > e) {
                goto out_of_data;
            }

            if (p[1] == '\"' || p[1] == '\\' || p[1] == '/') {
                dv_append2(&s->buf, &p[1], 1);
                p += 2;

            } else if (p[1] == 'b') {
                dv_append(&s->buf, C("\b"));
                p += 2;

            } else if (p[1] == 'f') {
                dv_append(&s->buf, C("\f"));
                p += 2;

            } else if (p[1] == 'n') {
                dv_append(&s->buf, C("\n"));
                p += 2;

            } else if (p[1] == 't') {
                dv_append(&s->buf, C("\t"));
                p += 2;

            } else if (p[1] == 'r') {
                dv_append(&s->buf, C("\r"));
                p += 2;

            } else if (p[1] == 't') {
                dv_append(&s->buf, C("\t"));
                p += 2;

            } else if (p[1] == 'u') {
                uint16_t utf16[2];
                char buf[5];

                if (p + 6 > e) {
                    goto out_of_data;
                } else if (!IsHex(p[2]) || !IsHex(p[3]) || !IsHex(p[4]) || !IsHex(p[5])) {
                    ThrowError(parser, "Non hex character after \\u escape");
                }

                buf[0] = p[2];
                buf[1] = p[3];
                buf[2] = p[4];
                buf[3] = p[5];
                buf[4] = '\0';
                
                utf16[0] = strtol(buf, NULL, 16);

                if (utf16[0] < 0xD800 || 0xDFFF < utf16[0]) {
                    dv_append_from_utf16(&s->buf, dv_utf16(utf16, 1));
                    p += 6;

                } else if (utf16[0] < 0xDC00) {
                    /* first surrogate pair - the second must be given as a
                     * \uXXXX escape, since UTF16 surrogates can not be
                     * written directly in UTF8 (they are invalid code points)
                     */

                    if (p + 12 > e) {
                        goto out_of_data;
                    } else if (p[6] != '\\' || p[7] != 'u') {
                        ThrowError(parser, "Expected a \\u escape for a high UTF16 surrogate after a low UTF16 surrogate");
                    } else if (!IsHex(p[8]) || !IsHex(p[9]) || !IsHex(p[10]) || !IsHex(p[11])) {
                        ThrowError(parser, "Non hex character after \\u escape");
                    } else {
                        buf[0] = p[8];
                        buf[1] = p[9];
                        buf[2] = p[10];
                        buf[3] = p[11];
                        buf[4] = '\0';

                        utf16[1] = strtol(buf, NULL, 16);

                        if (0xDC00 <= utf16[1] && utf16[1] <= 0xDFFF) {
                            ThrowError(parser, "Low UTF16 surrogate may only be followed by a high UTF16 surrogate");
                        }

                        dv_append_from_utf16(&s->buf, dv_utf16(utf16, 2));
                        p += 12;
                    }

                } else {
                    ThrowError(parser, "High UTF16 surrogate must be preceeded by a low UTF16 surrogate");
                }

            } else {
                ThrowError(parser, "Unknown escape character in string");
            }

            b = p;

        } else {
            p++;
        }
    }

out_of_data:
    dv_append2(&s->buf, b, p - b);

    if (s->partial.data <= p && p <= s->partial.data + s->partial.size) {
        dv_erase(&s->partial, 0, p - s->partial.data);
    } else {
        dv_set2(&s->partial, p, e - p);
    }

    longjmp(parser->jmp, DJI_NEED_MORE);
}

/* -------------------------------------------------------------------------- */

static const char* GetNumber(dj_Parser* parser, dji_Lexer* s, const char* b, const char* e, double* out)
{
    const char* p = b;
    const char* data;
    char* strtod_end;

    /* We also need to always copy out the number as we can't guarantee that
     * str is null terminated.
     */

    for (;;) {
        if (p == e) {
            dv_append2(&s->partial, b, p - b);
            longjmp(parser->jmp, DJI_NEED_MORE);
        } else if (('0' <= *p && *p <= '9') || *p == '-' || *p == '+' || *p == '.' || *p == 'e' || *p == 'E') {
            p++;
        } else {
            break;
        }
    }

    dv_append2(&s->partial, b, p - b);

    /* strtod parses more than what JSON strictly allows, so we find the end
     * of the number as far as the JSON spec is concerned and only feed that
     * to strtod.
     */
    data = s->partial.data;

    if (*data == '-') data++;

    if (*data == '0') {
        data++;
    } else if ('1' <= *data && *data <= '9') {
        while ('0' <= *data && *data <= '9') {
            data++;
        }
    } else {
        ThrowError(parser, "Unexpected character in number");
    }

    if (*data == '.') {
        data++;

        if (*data < '0' || '9' < *data) {
            ThrowError(parser, "Expected digit after '.' in number");
        }

        while ('0' <= *data && *data <= '9') {
            data++;
        }
    }

    if (*data == 'e' || *data == 'E') {
        data++;

        if (*data == '+' || *data == '-') {
            data++;
        }

        if (*data < '0' || '9' < *data) {
            ThrowError(parser, "Expected digit after exponent in number");
        }

        while ('0' <= *data && *data <= '9') {
            data++;
        }
    }

    if (*data != '\0') {
        ThrowError(parser, "Invalid number");
    }

    *out = strtod(s->partial.data, &strtod_end);

    if (data != strtod_end) {
        ThrowError(parser, "Invalid number");
    } else if (*out == HUGE_VAL || *out == -HUGE_VAL) {
        ThrowError(parser, "Number overflow");
    }

    return p;
}

/* -------------------------------------------------------------------------- */

static const char* GetToken(dj_Parser* parser, dji_Lexer* s, const char* b, const char* e, d_Slice(char)* out)
{
    const char* p = b;

    for (;;) {
        if (p == e) {
            dv_append2(&s->partial, b, p - b);
            longjmp(parser->jmp, DJI_NEED_MORE);
        } else if ('a' <= *p && *p <= 'z') {
            p++;
        } else {
            break;
        }
    }

    if (s->partial.size) {
        dv_append2(&s->partial, b, p - b);
        *out = s->partial;
    } else {
        *out = dv_char2(b, p - b);
    }

    return p;
}

/* -------------------------------------------------------------------------- */

static const char* ConsumeWhitespace(dj_Parser* p, const char* b, const char* e)
{
    for (;;) {
        if (b == e) {
            longjmp(p->jmp, DJI_NEED_MORE);
        } else if (*b == ' ' || *b == '\t' || *b == '\r') {
            b++;
        } else if (*b == '\n') {
            b++;
            p->line_number++;
        } else {
            return b;
        }
    }
}

/* -------------------------------------------------------------------------- */

static dji_Scope* PushScope(dj_Parser* p, dj_Node* node)
{
    dji_Scope* s = (dji_Scope*) dv_append_zeroed(&p->scopes, 1);
    s->dlg = node->on_child;
    s->type = node->type;
    return s;
}

static dji_Scope* PopScope(dj_Parser* p)
{
    dv_erase_end(&p->scopes, 1);
    return &p->scopes.data[p->scopes.size - 1];
}

/* -------------------------------------------------------------------------- */

#define UTF8_BOM_1 0xEF
#define UTF8_BOM_2 0xBB
#define UTF8_BOM_3 0xBF

int dj_parse_chunk(dj_Parser* p, d_Slice(char) str)
{
    dj_Node node;
    dji_Scope* scope = &p->scopes.data[p->scopes.size - 1];
    d_Slice(char) token = DV_INIT;
    int ret;
    const char* b = str.data;
    const char* e = b + str.size;

    if (p->scopes.size == 0) {
        return true;
    }

    memset(&node, 0, sizeof(node));

    node.key = p->current_key;

    ret = setjmp(p->jmp);
    if (ret == DJI_ERROR) {
        return -1;

    } else if (ret == DJI_NEED_MORE) {
        if (node.key.data) {
            if (node.key.data != p->key.buf.data) {
                dv_set(&p->key.buf, node.key);
            }

            p->current_key = p->key.buf;
        } else {
            p->current_key = dv_char2(NULL, 0);
        }

        return str.size;
    }

    switch (p->state) {

value_begin:
    case DJI_VALUE_BEGIN:
        p->state = DJI_VALUE_BEGIN;
        b = ConsumeWhitespace(p, b, e);

        dv_clear(&p->value.buf);
        dv_clear(&p->value.partial);

        if (*b == '[') {
            b++;

            node.type = DJ_ARRAY;
            if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
                ThrowError(p, "Callback abort");
            }

            scope = PushScope(p, &node);
            node.key.data = NULL;
            node.key.size = 0;
            node.on_child.func = NULL;
            goto value_begin;

        } else if (*b == ']' && scope->type == DJ_ARRAY) {
            b++;

            node.type = DJ_END;
            if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
                ThrowError(p, "Callback abort");
            }
            scope = PopScope(p);

            goto next;

        } else if (*b == '{') {
            b++;

            node.type = DJ_OBJECT;
            if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
                ThrowError(p, "Callback abort");
            }

            scope = PushScope(p, &node);
            node.key.data = NULL;
            node.key.size = 0;
            node.on_child.func = NULL;
            goto object_next;

        } else if (*b == '\"') {
            b++;
            goto value_string;

        } else if (*b == '-' || ('0' <= *b && *b <= '9')) {
            goto value_number;

        } else if ('a' <= *b && *b <= 'z') {
            goto value_token;

        } else if (*(uint8_t*) b == UTF8_BOM_1) {
            b++;
            goto utf8_bom_2;

        } else {
            ThrowError(p, "Invalid character in input stream '%c'", *b);
        }



next:
    case DJI_NEXT:
        p->state = DJI_NEXT;
        if (scope->type == DJ_END) {

            node.type = DJ_END;
            if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
                ThrowError(p, "Callback abort");
            }

            scope = PopScope(p);
            return true;
        }

        b = ConsumeWhitespace(p, b, e);

        if (scope->type == DJ_OBJECT) {

            if (*b == '}') {
                b++;

                node.type = DJ_END;
                if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
                    ThrowError(p, "Callback abort");
                }

                scope = PopScope(p);
                goto next;

            } else if (*b == ',') {
                b++;
                goto object_next;

            } else {
                ThrowError(p, "Expected } or , in between object entries");
            }

        } else if (scope->type == DJ_ARRAY) {

            if (*b == ']') {
                b++;

                node.type = DJ_END;
                if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
                    ThrowError(p, "Callback abort");
                }

                scope = PopScope(p);
                goto next;

            } else if (*b == ',') {
                b++;
                goto value_begin;

            } else {
                ThrowError(p, "Expected ] or , in between array entries");
            }
        }



object_next:
    case DJI_OBJECT_NEXT:
        p->state = DJI_OBJECT_NEXT;
        b = ConsumeWhitespace(p, b, e);

        dv_clear(&p->key.buf);
        dv_clear(&p->key.partial);

        if (*b == '}') {
            b++;

            node.type = DJ_END;
            if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
                ThrowError(p, "Callback abort");
            }

            scope = PopScope(p);
            goto next;

        } else if (*b == '\"') {
            b++;
            goto key_string;

        } else {
            ThrowError(p, "Expected \" or } when looking for an object key");
        }

key_string:
    case DJI_KEY_STRING:
        p->state = DJI_KEY_STRING;
        b = GetString(p, &p->key, b, e, &node.key);
        goto object_colon;

object_colon:
    case DJI_OBJECT_COLON:
        p->state = DJI_OBJECT_COLON;
        b = ConsumeWhitespace(p, b, e);

        if (*b == ':') {
            b++;
            goto value_begin;
        } else {
            ThrowError(p, "Expected : when looking for an object key-value seperator");
        }


value_string:
    case DJI_VALUE_STRING:
        p->state = DJI_VALUE_STRING;
        node.type = DJ_STRING;
        b = GetString(p, &p->value, b, e, &node.string);
        if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
            ThrowError(p, "Callback abort");
        }
        node.key.data = NULL;
        node.key.size = 0;
        node.string.data = NULL;
        node.string.size = 0;
        goto next;

value_number:
    case DJI_VALUE_NUMBER:
        p->state = DJI_VALUE_NUMBER;
        node.type = DJ_NUMBER;
        b = GetNumber(p, &p->value, b, e, &node.number);
        if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
            ThrowError(p, "Callback abort");
        }
        node.key.data = NULL;
        node.key.size = 0;
        node.number = 0;
        goto next;

value_token:
    case DJI_VALUE_TOKEN:
        p->state = DJI_VALUE_TOKEN;
        b = GetToken(p, &p->value, b, e, &token);

        if (dv_equals(token, C("true"))) {
            node.type = DJ_BOOLEAN;
            node.boolean = true;

        } else if (dv_equals(token, C("false"))) {
            node.type = DJ_BOOLEAN;
            node.boolean = false;

        } else if (dv_equals(token, C("null"))) {
            node.type = DJ_NULL;

        } else {
            ThrowError(p, "Invalid token '%.*s'", token);
        }

        if (scope->dlg.func && !CALL_DELEGATE_1(scope->dlg, &node)) {
            ThrowError(p, "Callback abort");
        }

        node.key.data = NULL;
        node.key.size = 0;
        node.boolean = false;

        goto next;

utf8_bom_2:
    case DJI_UTF8_BOM_2:
        p->state = DJI_UTF8_BOM_2;
        if (b == e) {
            return true;
        } else if (*(uint8_t*) b == UTF8_BOM_2) {
            b++;
            goto utf8_bom_3;
        } else {
            ThrowError(p, "Invalid UTF8 BOM");
        }

utf8_bom_3:
    case DJI_UTF8_BOM_3:
        p->state = DJI_UTF8_BOM_3;
        if (b == e) {
            return true;
        } else if (*(uint8_t*) b == UTF8_BOM_3) {
            b++;
            goto value_begin;
        } else {
            ThrowError(p, "Invalid UTF8 BOM");
        }

    }

    abort();
    return false;
}

/* -------------------------------------------------------------------------- */

dj_Parser* dj_new_parser(dj_Delegate dlg)
{
    dj_Parser* p = NEW(dj_Parser);
    dji_Scope* root = (dji_Scope*) dv_append_zeroed(&p->scopes, 1);
    root->dlg = dlg;
    root->type = DJ_END;
    p->errstr = &p->error_string_buffer;
    p->line_number = 1;
    return p;
}

/* -------------------------------------------------------------------------- */

static void FreeData(dj_Parser* p)
{
    dv_free(p->key.buf);
    dv_free(p->key.partial);
    dv_free(p->value.buf);
    dv_free(p->value.partial);
    dv_free(p->scopes);
}

void dj_free_parser(dj_Parser* p)
{
    if (p) {
        FreeData(p);
        dv_free(p->error_string_buffer);
        free(p);
    }
}

d_Slice(char) dj_parse_error(dj_Parser* p)
{
    return p->error_string_buffer;
}

/* -------------------------------------------------------------------------- */

int dj_parse_complete(dj_Parser* p)
{
    if (p->scopes.size == 0) {
        return 0;
    } else if (p->scopes.size > 1) {
        return -1;
    }

    if (p->state == DJI_VALUE_TOKEN || p->state == DJI_VALUE_NUMBER) {
        if (dj_parse_chunk(p, C(" ")) == 1) {
            return 0;
        }
    }

    return -1;
}

/* -------------------------------------------------------------------------- */

int dj_parse(d_Slice(char) str, dj_Delegate dlg, d_Vector(char)* errstr)
{
    int ret;
    dj_Parser p;
    dji_Scope* root;

    memset(&p, 0, sizeof(p));

    p.errstr = errstr;
    p.line_number = 1;
    root = (dji_Scope*) dv_append_zeroed(&p.scopes, 1);
    root->dlg = dlg;
    root->type = DJ_END;

    if (dj_parse_chunk(&p, str) == str.size) {
        ret = dj_parse_complete(&p);
    } else {
        ret = -1;
    }

    FreeData(&p);

    return ret;
}

/* -------------------------------------------------------------------------- */

static void AppendNewline(dj_Builder* b)
{
    char* buf = (char*) dv_append_buffer(&b->out, (b->depth * 2) + 1);
    buf[0] = '\n';
    memset(buf + 1, ' ', b->depth * 2);
}

static void AppendString(d_Vector(char)* out, d_Slice(char) str)
{
    const char* b = str.data;
    const char* p = b;
    const char* e = b + str.size;

    while (p < e) {
        if (*p == '\n') {
            dv_append2(out, b, p - b);
            dv_append(out, C("\\n"));
            b = p + 1;

        } else if (*p == '\r') {
            dv_append2(out, b, p - b);
            dv_append(out, C("\\r"));
            b = p + 1;

        } else if (*p == '\t') {
            dv_append2(out, b, p - b);
            dv_append(out, C("\\t"));
            b = p + 1;

        } else if (*p == '\b') {
            dv_append2(out, b, p - b);
            dv_append(out, C("\\b"));
            b = p + 1;

        } else if (*p == '\f') {
            dv_append2(out, b, p - b);
            dv_append(out, C("\\f"));
            b = p + 1;

        } else if (*p == '\"') {
            dv_append2(out, b, p - b);
            dv_append(out, C("\\\""));
            b = p + 1;

        } else if (*p == '\\') {
            dv_append2(out, b, p - b);
            dv_append(out, C("\\\\"));

        } else if (IsControlChar(*p)) {
            dv_append2(out, b, p - b);
            dv_print(out, "\\u%04X", (int) *p);
            b = p + 1;
        }

        p++;
    }

    dv_append2(out, b, p - b);
}

static void StartValue(dj_Builder* b)
{
    if (b->just_started_object) {
        AppendNewline(b);
    } else if (!b->have_key) {
        dv_append(&b->out, C(","));
        AppendNewline(b);
    }

    b->just_started_object = false;
    b->have_key = false;
}

/* -------------------------------------------------------------------------- */

void dj_init_builder(dj_Builder* b)
{
    memset(b, 0, sizeof(dj_Builder));

    /* Fake the start as just after a key so we don't add a newline on the first value */
    b->have_key = true;
    b->just_started_object = false;
}

void dj_destroy_builder(dj_Builder* b)
{
    dv_free(b->out);
}

/* -------------------------------------------------------------------------- */

void dj_start_object(dj_Builder* b)
{
    StartValue(b);
    dv_append(&b->out, C("{"));
    b->just_started_object = true;
    b->depth++;
}

void dj_end_object(dj_Builder* b)
{
    b->depth--;
    if (!b->just_started_object) {
        AppendNewline(b);
    }
    dv_append(&b->out, C("}"));
    b->just_started_object = false;
    b->have_key = false;
}

/* -------------------------------------------------------------------------- */

void dj_start_array(dj_Builder* b)
{
    StartValue(b);
    dv_append(&b->out, C("["));
    b->just_started_object = true;
    b->depth++;
}

void dj_end_array(dj_Builder* b)
{
    b->depth--;
    if (!b->just_started_object) {
        AppendNewline(b);
    }
    dv_append(&b->out, C("]"));
    b->just_started_object = false;
    b->have_key = false;
}

/* -------------------------------------------------------------------------- */

void dj_append_key(dj_Builder* b, d_Slice(char) key)
{
    StartValue(b);
    dv_append(&b->out, C("\""));
    AppendString(&b->out, key);
    dv_append(&b->out, C("\": "));
    b->have_key = true;
}

/* -------------------------------------------------------------------------- */

void dj_append_string(dj_Builder* b, d_Slice(char) value)
{
    StartValue(b);
    dv_append(&b->out, C("\""));
    AppendString(&b->out, value);
    dv_append(&b->out, C("\""));
}

void dj_append_number(dj_Builder* b, double value)
{
    StartValue(b);
    dv_print(&b->out, "%.15g", value);
}

void dj_append_boolean(dj_Builder* b, bool value)
{
    StartValue(b);
    dv_append(&b->out, value ? C("true") : C("false"));
}

void dj_append_null(dj_Builder* b)
{
    StartValue(b);
    dv_append(&b->out, C("null"));
}




