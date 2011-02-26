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

#include "vector.h"

DVECTOR_INIT(char, char);
DVECTOR_INIT(char_vector, d_Vector(char));
DVECTOR_INIT(char_slice, d_Slice(char));

/* ------------------------------------------------------------------------- */

DMEM_INLINE void dv_clear_string_vector(d_Vector(char_vector)* vec)
{
    int i;
    for (i = 0; i < vec->size; i++) {
        dv_free(vec->data[i]);
    }
    dv_clear(vec);
}

/* Macro to wrap char slices/vectors for printing with printf - use "%.*s" in
 * the format string.
 */
#define DV_PRI(STR) (STR).size, (STR).data

/* Returns whether the char slice/vector 'VEC' begins with the char slice
 * 'TEST'.
 */
#define dv_begins_with(VEC, TEST) ((VEC).size > (TEST).size && 0 == memcmp((VEC).data, (TEST).data, (TEST).size * dv_datasize(VEC)))

/* Returns whether the char slice/vector 'VEC' ends with the char slice
 * 'TEST'.
 */
#define dv_ends_with(VEC, TEST)   ((VEC).size > (TEST).size && 0 == memcmp((VEC).data + (VEC).size - (TEST).size, (TEST).data, (TEST).size * dv_datasize(VEC)))

/* ------------------------------------------------------------------------- */

/* Macro to convert char string literals into a d_Slice(char) */
#define C(STR) dv_char2(STR, sizeof(STR) - 1)

/* Macro to convert a d_Slice(char) to a std::string */
#define dv_to_string(slice) std::string((slice).data, (slice).data + (slice).size)

/* ------------------------------------------------------------------------- */

/* Appends a base64 encoded version of 'from' to 'to' */
DMEM_API void dv_append_base64_encoded(d_Vector(char)* to, d_Slice(char) from);

/* Appends a hex encoded version of 'from' to 'to' */
DMEM_API void dv_append_hex_encoded(d_Vector(char)* to, d_Slice(char) from);

/* Appends a hex decoded version of 'from' to 'to' */
DMEM_API void dv_append_hex_decoded(d_Vector(char)* to, d_Slice(char) from);

/* Appends a quoted version of 'from' to 'to' */
DMEM_API void dv_append_quoted(d_Vector(char)* to, d_Slice(char) from, char quote);

DMEM_API void dv_append_url_decoded(d_Vector(char)* to, d_Slice(char) from);
DMEM_API void dv_append_url_encoded(d_Vector(char)* to, d_Slice(char) from);

DMEM_API d_Slice(char) dv_split_left(d_Slice(char)* from, char sep);

/* ------------------------------------------------------------------------- */

/* Returns if there is a next full line and the slice in 'line' - 'line' must
 * still point to the previous line and must be null initialised before
 * grabbing the first line.
 */
DMEM_API bool dv_has_next_line(d_Slice(char) s, d_Slice(char)* line);

/* Returns if there is a next full line and the slice in 'line' - 'used' is
 * used to keep track of how far through the string we are. It must be set to
 * 0 before grabbing the first line.
 */
DMEM_API bool dv_has_next_line2(d_Slice(char) s, d_Slice(char)* line, int* used);

/* ------------------------------------------------------------------------- */

/* Appends a pretty printed hex data dump of the data in slice. This can be
 * optionally coloured using inline ansi terminal colour commands.
 */
DMEM_API void dv_append_hex_dump(d_Vector(char)* s, d_Slice(char) data, bool colors);

/* ------------------------------------------------------------------------- */

typedef struct dv_tokenizer dv_tokenizer;

struct dv_tokenizer {
    const uint8_t* next;
    const uint8_t* end;
    uint32_t map[8];
};

/* Loads a tokenizer for the requested string and delimiters. */
DMEM_API void dv_load_token(dv_tokenizer* tok, d_Slice(char) str, const char* delim);

/* Returns the next tokenised slice from the str and tokenised using the
 * delimiters from the call to dv_load_token. Will return an empty slice when
 * all tokens have been consumed.
 */
DMEM_API d_Slice(char) dv_next_token(dv_tokenizer* tok);

DMEM_API void dv_generate_uuid(d_Vector(char)* out);

/* ------------------------------------------------------------------------- */

/* Returns a slice for the null terminated string 'str' */
DMEM_INLINE d_Slice(char) dv_char(const char* str)
{
    d_Slice(char) ret;
    ret.size = str ? (int) strlen(str) : 0;
    ret.data = (char*) str;
    return ret;
}

#ifdef __cplusplus
/* Returns a slice for the null terminated string 'str' - overload for cpp */
DMEM_INLINE d_Slice(char) dv_char(char* str)
{
    d_Slice(char) ret;
    ret.size = str ? (int) strlen(str) : 0;
    ret.data = (char*) str;
    return ret;
}

/* Returns a slice for the null terminated string 'str' - overload for
 * std::string and compatible interfaces
 */
template <class T>
DMEM_INLINE d_Slice(char) dv_char(const T& str)
{
    d_Slice(char) ret;
    ret.size = (int) str.size();
    ret.data = str.c_str();
    return ret;
}
#endif

/* Returns a slice for the string 'str' of size 'size' */
DMEM_INLINE d_Slice(char) dv_char2(const char* str, size_t size)
{
    d_Slice(char) ret;
    ret.size = (int) size;
    ret.data = (char*) str;
    return ret;
}

/* ------------------------------------------------------------------------- */

/* Returns the slice left of index from (with -from being the index from the end)
 *
 * For example:
 * - dv_left(C("foobar123"), -3) returns "foobar"
 * - dv_left(C("foobar123"), 3) returns "foo"
 */
DMEM_INLINE d_Slice(char) dv_left(d_Slice(char) str, int from)
{
    if (from >= 0) {
        return dv_char2(str.data, from);
    } else {
        return dv_char2(str.data, str.size + from);
    }
}

/* Returns the slice right of index from (with -from being the index from the end).
 *
 * For example:
 * - dv_right(C("foobar123"), -3) returns "123"
 * - dv_right(C("foobar123"), 3) returns "bar123"
 */
DMEM_INLINE d_Slice(char) dv_right(d_Slice(char) str, int from)
{
    if (from >= 0) {
        return dv_char2(str.data + from, str.size - from);
    } else {
        return dv_char2(str.data + str.size + from, -from);
    }
}

DMEM_INLINE d_Slice(char) dv_slice(d_Slice(char) str, int from, int size)
{
    if (from >= 0) {
        return dv_char2(str.data + from, size);
    } else {
        return dv_char2(str.data + str.size + from, size);
    }
}

/* ------------------------------------------------------------------------- */

/* Converts the string slice to a number - see strtod for valid values */
DMEM_API double dv_to_number(d_Slice(char) value);
DMEM_API int dv_to_integer(d_Slice(char) value, int radix, int def);

/* Converts the boolean slice to a number - valid true values are "true",
 * "TRUE", "1", etc */
DMEM_API bool dv_to_boolean(d_Slice(char) value);

DMEM_API d_Slice(char) dv_strip_whitespace(d_Slice(char) str);

/* ------------------------------------------------------------------------- */

/* Appends a sprintf formatted string to 's' */
DMEM_INLINE int dv_print(d_Vector(char)* s, const char* format, ...) DMEM_PRINTF(2, 3);
DMEM_API int dv_vprint(d_Vector(char)* s, const char* format, va_list ap) DMEM_PRINTF(2, 0);

DMEM_INLINE int dv_print(d_Vector(char)* s, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    return dv_vprint(s, format, ap);
}

/* ------------------------------------------------------------------------- */

DMEM_INLINE int dv_find_char(d_Slice(char) str, char ch)
{
    char* p = (char*) memchr(str.data, ch, str.size);
    return p ? p - str.data : -1;
}

DMEM_INLINE int dv_find_last_char(d_Slice(char) str, char ch)
{
    char* p = (char*) memrchr(str.data, ch, str.size);
    return p ? p - str.data : -1;
}

DMEM_INLINE int dv_find_string(d_Slice(char) str, d_Slice(char) val)
{
    char* p = (char*) memmem(str.data, str.size, val.data, val.size);
    return p ? p - str.data : -1;
}

/* ------------------------------------------------------------------------- */

DMEM_API void dv_replace_string(d_Vector(char)* s, d_Slice(char) from, d_Slice(char) to);

DMEM_API void dv_relative_path(d_Vector(char)* path, d_Slice(char) relative);
DMEM_API void dv_append_normalised_path(d_Vector(char)* out, d_Slice(char) path);

