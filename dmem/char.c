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

/* needed for NAN */
#define _ISOC99_SOURCE
#define DMEM_LIBRARY

#include <dmem/char.h>
#include <uuid/uuid.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#ifdef _MSC_VER
#include <malloc.h>
#define alloca _alloca
#else
#include <alloca.h>
#endif


#ifndef va_copy
#   ifdef _MSC_VER
#       define va_copy(d,s) d = s
#   elif defined __GNUC__
#       define va_copy(d,s)	__builtin_va_copy(d,s)
#   else
#       error
#   endif
#endif

#define dv_isspace(c) ((c) == '\n' || (c) == '\t' || (c) == ' ' || (c) == '\r')

/* ------------------------------------------------------------------------- */

int dv_vprint(d_Vector(char)* v, const char* format, va_list ap)
{
    dv_reserve(v, v->size + 1);

    for (;;) {

        int ret;

        char* buf = (char*) v->data + v->size;
        int bufsz = (int) ((uint64_t*) v->data)[-1] - v->size;

        va_list aq;
        va_copy(aq, ap);

        /* We initialise buf[bufsz] to \0 to detect when snprintf runs out of
         * buffer by seeing whether it overwrites it.
         */
        buf[bufsz] = '\0';
        ret = vsnprintf(buf, bufsz + 1, format, aq);

        if (ret > bufsz) {
            /* snprintf has told us the size of buffer required (ISO C99
             * behavior)
             */
            dv_reserve(v, v->size + ret);

        } else if (ret >= 0) {
            /* success */
            v->size += ret;
            return ret;

        } else if (buf[bufsz] != '\0') {
            /* snprintf has returned an error but has written to the end of the
             * buffer (MSVC behavior). The buffer is not large enough so grow
             * and retry. This can also occur with a format error if it occurs
             * right on the boundary, but then we grow the buffer and can
             * figure out its an error next time around. 
             */
            dv_reserve(v, v->size + bufsz + 1);

        } else {
            /* snprintf has returned an error but has not written to the last
             * character in the buffer. We have a format error.
             */
            return -1;
        }
    }
}

/* ------------------------------------------------------------------------- */

static const char g_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void ToBase64(uint8_t src[3], char dest[4]) 
{
    /* Input:  xxxx xxxx yyyy yyyy zzzz zzzz
     * Output: 00xx xxxx 00xx yyyy 00yy yyzz 00zz zzzz
     */
    dest[0] = ((src[0] >> 2) & 0x3F);

    dest[1] = ((src[0] << 4) & 0x30)
            | ((src[1] >> 4) & 0x0F);

    dest[2] = ((src[1] << 2) & 0x3C) 
            | ((src[2] >> 6) & 0x03);

    dest[3] = (src[2] & 0x3F);

    dest[0] = g_base64[(int) dest[0]];
    dest[1] = g_base64[(int) dest[1]];
    dest[2] = g_base64[(int) dest[2]];
    dest[3] = g_base64[(int) dest[3]];
}

void dv_append_base64_encoded(d_Vector(char)* v, d_Slice(char) from)
{
    uint8_t* up = (uint8_t*) from.data;
    uint8_t* uend = up + from.size;
    char* dest;

    dest = dv_append_buffer(v, ((from.size + 2) / 3) * 4);

    while (up + 3 <= uend) {
        ToBase64(up, dest);
        up += 3;
        dest += 4;
    }

    if (up + 2 == uend) {
        uint8_t pad[3];
        pad[0] = up[0];
        pad[1] = up[1];
        pad[2] = 0;

        ToBase64(pad, dest);
        dest[3] = '=';

    } else if (up + 1 == uend) {
        uint8_t pad[3];
        pad[0] = up[0];
        pad[1] = 0;
        pad[2] = 0;

        ToBase64(pad, dest);
        dest[2] = '=';
        dest[3] = '=';
    }
}

/* -------------------------------------------------------------------------- */

void dv_append_hex_decoded(d_Vector(char)* to, d_Slice(char) from)
{
    int i;
    uint8_t* ufrom = (uint8_t*) from.data;
    uint8_t* dest = (uint8_t*) dv_append_buffer(to, from.size / 2);

    if (from.size % 2 != 0) {
        goto err;
    }

    for (i = 0; i < from.size / 2; ++i) {
        uint8_t hi = ufrom[2*i];
        uint8_t lo = ufrom[2*i + 1];

        if ('0' <= hi && hi <= '9') {
            hi = hi - '0';
        } else if ('a' <= hi && hi <= 'f') {
            hi = hi - 'a' + 10;
        } else if ('A' <= hi && hi <= 'F') {
            hi = hi - 'A' + 10;
        } else {
            goto err;
        }

        if ('0' <= lo && lo <= '9') {
            lo = lo - '0';
        } else if ('a' <= lo && lo <= 'f') {
            lo = lo - 'a' + 10;
        } else if ('A' <= lo && lo <= 'F') {
            lo = lo - 'A' + 10;
        } else {
            goto err;
        }


        *(dest++) = (hi << 4) | lo;
    }

    return;

err:
    dv_erase_end(to, from.size / 2);
}

/* -------------------------------------------------------------------------- */

void dv_append_hex_encoded(d_Vector(char)* to, d_Slice(char) from)
{
    int i;
    uint8_t* ufrom = (uint8_t*) from.data;
    uint8_t* dest = (uint8_t*) dv_append_buffer(to, from.size * 2);

    for (i = 0; i < from.size; ++i) {
        uint8_t hi = ufrom[i] >> 4;
        uint8_t lo = ufrom[i] & 0x0F;

        if (hi < 10) {
            hi += '0';
        } else {
            hi += 'a' - 10;
        }

        if (lo < 10) {
            lo += '0';
        } else {
            lo += 'a' - 10;
        }

        *(dest++) = hi;
        *(dest++) = lo;
    }
}

/* -------------------------------------------------------------------------- */

void dv_append_url_decoded(d_Vector(char)* to, d_Slice(char) from)
{
    const char* b = from.data;
    const char* e = from.data + from.size;
    const char* p = b;

    while (p < e) {
        if (*p == '%') {
            dv_append2(to, b, p - b);

            /* if we have an incomplete or invalid espace then just ignore it */
            if (p + 3 > e) {
                return;
            }

            dv_append_hex_decoded(to, dv_char2(p + 1, 2));

            p += 3;
            b = p;

        } else if (*p == '+') {
            dv_append2(to, b, p - b);
            dv_append(to, C(" "));
            p++;
            b = p;

        } else {
            p++;
        }
    }

    dv_append2(to, b, p - b);
}

/* -------------------------------------------------------------------------- */

void dv_append_url_encoded(d_Vector(char)* to, d_Slice(char) from)
{
    const char* b = from.data;
    const char* e = b + from.size;
    const char* p = b;

    while (p < e) {
        if (    ('a' <= *p && *p <= 'z')
             || ('A' <= *p && *p <= 'Z')
             || ('0' <= *p && *p <= '9')
             || *p == '-'
             || *p == '_'
             || *p == '~'
             || *p == '.') {
            p++;
        } else {
            dv_append2(to, b, p - b);
            dv_print(to, "%%%02x", *(unsigned char*) p);
            p++;
            b = p;
        }
    }

    dv_append2(to, b, p - b);
}

/* -------------------------------------------------------------------------- */

bool dv_has_next_line2(d_Slice(char) s, d_Slice(char)* line, int* used)
{
    const char* start = s.data + *used;
    const char* end = s.data + s.size;
    const char* nl = (const char*) memchr(start, '\n', end - start);

    if (nl) {
        *used = (int) (nl + 1 - s.data);

        if (nl > start && nl[-1] == '\r') {
            nl--;
        }

        *line = dv_char2(start, (int) (nl - start));
        return true;

    } else {
        line->data = NULL;
        line->size = 0;
        return false;
    }
}

bool dv_has_next_line(d_Slice(char) s, d_Slice(char)* line)
{
    int used = 0;
    const char* data = s.data;

    if (line->data) {
        used = line->data + line->size - data;
        if (used < s.size && data[used] == '\r') used++;
        if (used < s.size && data[used] == '\n') used++;
    }

    return dv_has_next_line2(s, line, &used);
}


/* -------------------------------------------------------------------------- */

#define test(map, val) (map[(val) >> 5] & (1 << ((val) & 31)))
#define set(map, val) map[(val) >> 5] |= 1 << ((val) & 31)

void dv_load_token(dv_tokenizer* tok, d_Slice(char) s, const char* delim)
{
    const uint8_t* udelim = (const uint8_t*) delim;
    int i, ch;

    for (i = 0; i < 8; i++) {
        tok->map[i] = 0;
    }

    while ((ch = *(udelim++)) != 0) {
        set(tok->map, ch);
    }

    tok->next = (const uint8_t*) s.data;
    tok->end = tok->next + s.size;
}

d_Slice(char) dv_next_token(dv_tokenizer* tok)
{
    d_Slice(char) ret;
    const uint8_t* p = tok->next;

    while (p < tok->end && test(tok->map, *p)) {
        p++;
    }

    ret.data = (char*) p;

    while (p < tok->end && !test(tok->map, *p)) {
        p++;
    }

    ret.size = (int) ((char*) p - ret.data);
    tok->next = p;

    return ret;
}

/* -------------------------------------------------------------------------- */

#ifndef min
#   define min(x, y) ((x < y) ? (x) : (y))
#endif

#define dv_isprint(ch) (' ' <= ch && ch <= '~')

#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define NORMAL  "\033[m"
#define GREY    "\033[1;30m"

void dv_append_hex_dump(d_Vector(char)* s, d_Slice(char) data, bool colors)
{
    int i, j;
    uint8_t* buf = (uint8_t*) data.data;

    for (i = 0; i < data.size; i += 16) {
        int spaces = 40;
        int end = min(i + 16, data.size);

        dv_print(s, "\t%s0x%04x    ", colors ? CYAN : "", i);

        for (j = i; j < end; j++) {
            spaces -= 2;

            if (colors) {
                dv_print(s, "%s%02x",
                        dv_isprint(buf[j]) ? NORMAL : RED,
                        (int) buf[j]);
            } else {
                dv_print(s, "%02x", (int) buf[j]);
            }

            if (j > 0 && j % 2) {
                spaces -= 1;
                dv_append(s, C(" "));
            }
        }

        if (spaces > 0) {
            char* dest = dv_append_buffer(s, spaces); 
            memset(dest, ' ', spaces);
        }

        for (j = i; j < end; j++) {
            if (dv_isprint(buf[j])) {
                dv_print(s, "%s%c", colors ? NORMAL : "", (int) buf[j]);
            } else {
                dv_print(s, "%s.", colors ? RED : "");
            }
        }

        dv_print(s, "%s\n", colors ? NORMAL : "");
    }
}

/* ------------------------------------------------------------------------- */

double dv_to_number(d_Slice(char) value)
{
    char* end;
    double ret;
    char* buf = (char*) alloca(value.size + 1);

    if (value.size == 0) return NAN;

    memcpy(buf, value.data, value.size);
    buf[value.size] = '\0';
    ret = strtod(buf, &end);

    if (*end == '\0' || dv_isspace(*end))
      return ret;
    else
      return 0;
}

/* ------------------------------------------------------------------------- */

int dv_to_integer(d_Slice(char) value, int radix, int defvalue)
{
    long ret;
    char* end;
    char* buf = (char*) alloca(value.size + 1);

    memcpy(buf, value.data, value.size);
    buf[value.size] = '\0';
    ret = strtol(buf, &end, radix);
    
    if (*end == '\0' || dv_isspace(*end))
      return ret;
    else
      return defvalue;
}

/* ------------------------------------------------------------------------- */

bool dv_to_boolean(d_Slice(char) value)
{
    if (dv_equals(value, C("TRUE"))) {
        return true;
    } else if (dv_equals(value, C("true"))) {
        return true;
    } else if (dv_equals(value, C("1"))) {
        return true;
    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------- */

d_Slice(char) dv_strip_whitespace(d_Slice(char) s)
{
    char* p = s.data + s.size;

    while (p >= s.data && dv_isspace(p[-1])) {
        p--;
    }

    s.size = p - s.data;
    p = s.data;
    while (p < s.data + s.size && dv_isspace(p[0])) {
        p++;
    }

    s.size -= p - s.data;
    s.data = p;
    return s;
}

/* ------------------------------------------------------------------------- */

void dv_generate_uuid(d_Vector(char)* out)
{
    uuid_t id;
    uuid_generate(id);
    dv_append_hex_encoded(out, dv_char2((char*) id, sizeof(id)));
}

/* ------------------------------------------------------------------------- */

void dv_replace_string(d_Vector(char)* s, d_Slice(char) from, d_Slice(char) to)
{
    int idx = 0;
    for (;;) {
        int find = dv_find_string(dv_right(*s, idx), from);

        if (find < 0) {
            return;
        }

        idx += find;

        if (from.size >= to.size) {
            memcpy(&s->data[idx], to.data, to.size);
            dv_erase(s, idx + to.size, from.size - to.size);
        } else {
            memcpy(&s->data[idx], to.data, from.size);
            dv_insert(s, idx + from.size, dv_right(to, from.size));
        }

        idx += to.size;
    }
}

/* ------------------------------------------------------------------------- */

d_Slice(char) dv_split_left(d_Slice(char)* from, char sep)
{
    d_Slice(char) ret;
    int idx = dv_find_char(*from, sep);

    if (idx < 0) {
        ret = *from;
        *from = dv_right(*from, from->size);
    } else {
        ret = dv_left(*from, idx);
        *from = dv_right(*from, idx + 1);
    }

    return ret;
}

/* ------------------------------------------------------------------------- */

#ifndef NDEBUG
static void TestPath(void)
{
    d_Vector(char) p = DV_INIT;

#define TEST(from, to)                                                      \
    dv_clear(&p);                                                           \
    dv_append_normalised_path(&p, C(from));                                 \
    assert(dv_equals(p, C(to)))

    TEST("/", "/");
    TEST("/..", "/");
    TEST("/../", "/");
    TEST("//", "/");
    TEST("/./", "/");
    TEST("/.//", "/");
    TEST("/foo/..", "/");
    TEST("/foo/../", "/");
    TEST("/foo/.", "/foo");
    TEST("/foo/./", "/foo");
    TEST("/foo/bar", "/foo/bar");
    TEST("/foo/bar/", "/foo/bar");
    TEST("/foo//bar", "/foo/bar");
    TEST("/foo/./bar", "/foo/bar");
    TEST("/foo/../bar", "/bar");
    TEST("/foo/../bar/", "/bar");
    TEST("/foo/bar/..", "/foo");

    dv_free(p);

#undef TEST
}
#endif

static void RelativePath(d_Vector(char)* out, int off, d_Slice(char) path)
{
    char *begin, *p, *end;

#ifndef NDEBUG
    static int test = 0;

    if (test++ == 0) {
        TestPath();
    }

#endif

    /* Make sure it starts with a / */
    if (out->size > off && out->data[off] != '/') {
        dv_insert(out, off, C("/"));
    }

    /* Make sure it has a / separator */
    if (!dv_begins_with(path, C("/"))) {
        dv_append(out, C("/"));
    }

    dv_append(out, path);

    begin = out->data + off;
    p = begin + 1;
    end = out->data + out->size;

    while (p < end) {

        /* p is set to the beginning of the current component (eg with /foo it
         * points to f)
         */

        /* Remove repeating slashes // */
        if (p[0] == '/') {
            dv_erase(out, p - out->data, 1);
            end -= 1;


            /* Remove unnecessary /./ path components. Since p points to the dot
             * this removes the dot and the preceding slash.
             */
        } else if (p[0] == '.' && (p + 1 == end || p[1] == '/')) {
            dv_erase(out, p - 1 - out->data, 2);
            end -= 2;


            /* Remove unnecessary /foo/../ path components. Calculate prev which
             * points to the beginning of the preceding component. If there is no
             * preceding component e.g. with /../foo then prev will point to the
             * beginning of the same component.  We then remove from the
             * proceeding component to the end of the current component and the
             * proceeding slash.
             */
        } else if (p[0] == '.' && p[1] == '.' && (p + 2 == end || p[2] == '/')) {

            char* prev = p;

            if (prev - 1 > begin) {
                prev--;

                while (prev[-1] != '/') {
                    prev--;
                }
            }

            dv_erase(out, prev - 1 - out->data, p - prev + 3);
            end -= p - prev + 3;
            p = prev;


            /* Find the end of the current component. */
        } else {
            p = memchr(p, '/', end - p);

            if (p) {
                p++;
            } else {
                break;
            }
        }
    }

    /* After removing /. and /../foo components we may end up with an empty
     * string.
     */
    if (out->size == off) {
        dv_append(out, C("/"));
    }

    /* Remove trailing slashes */
    if (out->size - off > 1 && end[-1] == '/') {
        dv_erase_end(out, 1);
    }
}

void dv_relative_path(d_Vector(char)* path, d_Slice(char) relative)
{ RelativePath(path, 0, relative); }

void dv_append_normalised_path(d_Vector(char)* out, d_Slice(char) path)
{ RelativePath(out, out->size, path); }

