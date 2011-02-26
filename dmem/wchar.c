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
#include <dmem/wchar.h>
#include <assert.h>

static void ReplaceUtf8(uint8_t** dp, const uint16_t** sp)
{
    /* Insert the replacement character */
    (*dp)[0] = 0xEF;
    (*dp)[1] = 0xBF;
    (*dp)[2] = 0xBD;
    *dp += 3;
    *sp += 1;
}

/* Requires a mininum of 3 * (n + 1) characters in dest */
int utf16_to_utf8(uint8_t* dest, int dn, const uint16_t* src, int sn)
{
    uint8_t* dp = dest;
    const uint16_t* sp = src;
    const uint16_t* send = src + sn;

    (void) dn;
    assert(dn >= 3 * (sn + 1));

    while (sp < send) {
        if (sp[0] < 0x80) {
            /* 1 chars utf8, 1 wchar utf16 (US-ASCII)
             * UTF32:  00000000 00000000 00000000 0xxxxxxx
             * Source: 00000000 0xxxxxxx
             * Dest:   0xxxxxxx
             */
            dp[0] = (uint8_t) sp[0];
            dp += 1;
            sp += 1;
        } else if (sp[0] < 0x800) {
            /* 2 chars utf8, 1 wchar utf16
             * UTF32:  00000000 00000000 00000yyy xxxxxxxx
             * Source: 00000yyy xxxxxxxx
             * Dest:   110yyyxx 10xxxxxx
             */
            dp[0] = (uint8_t) (0xC0 | ((sp[0] >> 6) & 0x1F));
            dp[1] = (uint8_t) (0x80 | (sp[0] & 0x3F));
            dp += 2;
            sp += 1;
        } else if (sp[0] < 0xD800) {
            /* 3 chars utf8, 1 wchar utf16
             * UTF32:  00000000 00000000 yyyyyyyy xxxxxxxx
             * Source: yyyyyyyy xxxxxxxx
             * Dest:   1110yyyy 10yyyyxx 10xxxxxx
             */
            dp[0] = (uint8_t) (0xE0 | ((sp[0] >> 12) & 0x0F));
            dp[1] = (uint8_t) (0x80 | ((sp[0] >> 6) & 0x3F));
            dp[2] = (uint8_t) (0x80 | (sp[0] & 0x3F));
            dp += 3;
            sp += 1;
        } else if (sp[0] < 0xDC00) {
            /* 4 chars utf8, 2 wchars utf16
             * 0xD8 1101 1000
             * 0xDB 1101 1011
             * 0xDC 1101 1100
             * 0xDF 1101 1111
             * UTF32:  00000000 000zzzzz yyyyyyyy xxxxxxxx
             * Source: 110110zz zzyyyyyy 110111yy xxxxxxxx
             * Dest:   11110zzz 10zzyyyy 10yyyyxx 10xxxxxx
             * UTF16 data is shifted by 0x10000
             */
            if (sp + 1 > send) {                                /* Check that we can have enough bytes */
                ReplaceUtf8(&dp, &sp);
            } else if (!(0xDC00 <= sp[1] && sp[1] <= 0xDFFF)) { /* Check for a valid surrogate */
                ReplaceUtf8(&dp, &sp);
            } else {
                uint32_t u32 = ((((uint32_t) sp[0]) << 10) & 0x0FFC00) | (((uint32_t) sp[1]) & 0x3FF);
                u32 += 0x10000;
                dp[0] = (uint8_t) (0xF0 | ((u32 >> 18) & 0x03));
                dp[1] = (uint8_t) (0x80 | ((u32 >> 12) & 0x3F));
                dp[2] = (uint8_t) (0x80 | ((u32 >> 6) & 0x3F));
                dp[3] = (uint8_t) (0x80 | (u32 & 0x3F));
                dp += 4;
                sp += 2;
            }
        } else {
            /* 3 chars utf8, 1 wchar utf16
             * UTF32:  00000000 00000000 yyyyyyyy xxxxxxxx
             * Source: yyyyyyyy xxxxxxxx
             * Dest:   1110yyyy 10yyyyxx 10xxxxxx
             */
            dp[0] = (uint8_t) (0xE0 | ((sp[0] >> 12) & 0x0F));
            dp[1] = (uint8_t) (0x80 | ((sp[0] >> 6) & 0x3F));
            dp[2] = (uint8_t) (0x80 | (sp[0] & 0x3F));
            dp += 3;
            sp += 1;
        }
    }
    assert(dp - dest <= (int) (3 * (sn + 1)));
    return (int) (dp - dest);
}

void dv_append_from_utf16(d_Vector(char)* str, d_Slice(wchar) from)
{
    int dn = 3 * (from.size + 1);
    uint8_t* dest = (uint8_t*) dv_append_buffer(str, dn);
    int used = utf16_to_utf8(dest, dn, from.data, from.size);
    dv_erase_end(str, dn - used);
}

static void ReplaceUtf16(uint16_t** dp, const uint8_t** sp, int srcskip)
{
    /* Insert the replacement character */
    **dp = 0xFFFD;
    *dp += 1;
    *sp += srcskip;
}

int utf8_to_utf16(uint16_t* dest, int dn, const uint8_t* src, int sn)
{
    uint16_t* dp = dest;
    const uint8_t* sp = src;
    const uint8_t* send = src + sn;

    (void) dn;
    assert(dn >= sn);

    while (sp < send) {
        if (sp[0] < 0x80) {
            /* 1 char utf8, 1 wchar utf16 (US-ASCII)
             * UTF32:  00000000 00000000 00000000 0xxxxxxx
             * Source: 0xxxxxxx
             * Dest:   00000000 0xxxxxxx
             */
            dp[0] = sp[0];
            dp += 1;
            sp += 1;
        } else if (sp[0] < 0xC0) {
            /* Multi-byte data without start */
            ReplaceUtf16(&dp, &sp, 1);
        } else if (sp[0] < 0xE0) {
            /* 2 chars utf8, 1 wchar utf16
             * UTF32:  00000000 00000000 00000yyy xxxxxxxx
             * Source: 110yyyxx 10xxxxxx
             * Dest:   00000yyy xxxxxxxx
             * Overlong: Require 1 in some y or top bit of x
             */
            if (sp + 2 > send) {                  /* Check that we have enough bytes */
                ReplaceUtf16(&dp, &sp, 1);
            } else if ((sp[1] & 0xC0) != 0x80) {  /* Check continuation byte */
                ReplaceUtf16(&dp, &sp, 1);
            } else if ((sp[1] & 0x1E) == 0) {     /* Check for overlong encoding */
                ReplaceUtf16(&dp, &sp, 2);
            } else {
                dp[0] = ((((uint16_t) sp[0]) & 0x1F) << 6)
                      | (((uint16_t) sp[1]) & 0x3F);
                dp += 1;
                sp += 2;
            }
        } else if (sp[0] < 0xF0) {
            /* 3 chars utf8, 1 wchar utf16
             * UTF32:  00000000 00000000 yyyyyyyy xxxxxxxx
             * Source: 1110yyyy 10yyyyxx 10xxxxxx
             * Dest:   yyyyyyyy xxxxxxxx
             * Overlong: Require 1 in one of the top 5 bits of y
             */
            if (sp + 3 > send) {                  /* Check that we have enough bytes */
                ReplaceUtf16(&dp, &sp, 1);
            } else if ((sp[1] & 0xC0) != 0x80) {  /* Check continuation byte */
                ReplaceUtf16(&dp, &sp, 1);
            } else if ((sp[2] & 0xC0) != 0x80) {  /* Check continuation byte */
                ReplaceUtf16(&dp, &sp, 1);
            } else if ((sp[0] & 0x0F) == 0 && (sp[1] & 0x20) == 0) { /* Check for overlong encoding */
                ReplaceUtf16(&dp, &sp, 3);
            } else {
                dp[0] = ((((uint16_t) sp[0]) & 0x0F) << 12)
                      | ((((uint16_t) sp[1]) & 0x3F) << 6)
                      | (((uint16_t) sp[2]) & 0x3F);
                dp += 1;
                sp += 3;
            }
        } else if (sp[0] < 0xF8) {
            /* 4 chars utf8, 2 wchars utf16
             * UTF32:  00000000 000zzzzz yyyyyyyy xxxxxxxx
             * Source: 110110zz zzyyyyyy 110111yy xxxxxxxx
             * Dest:   11110zzz 10zzyyyy 10yyyyxx 10xxxxxx
             * Overlong: Check UTF32 value
             * UTF16 data is shifted by 0x10000
             */
            if (sp + 4 > send) {                  /* Check that we have enough bytes */
                ReplaceUtf16(&dp, &sp, 1);
            } else if ((sp[1] & 0xC0) != 0x80) {  /* Check continuation byte */
                ReplaceUtf16(&dp, &sp, 1);
            } else if ((sp[2] & 0xC0) != 0x80) {  /* Check continuation byte */
                ReplaceUtf16(&dp, &sp, 1);
            } else if ((sp[3] & 0xC0) != 0x80) {  /* Check continuation byte */
                ReplaceUtf16(&dp, &sp, 1);
            } else {
                uint32_t u32  = ((((uint16_t) sp[0]) & 0x07) << 18)
                              | ((((uint16_t) sp[1]) & 0x3F) << 12)
                              | ((((uint16_t) sp[2]) & 0x3F) << 6)
                              | (((uint16_t) sp[3]) & 0x3F);

                /* Check for overlong or too long encoding */
                if (u32 < 0x10000 || u32 > 0x10FFFF) {
                    ReplaceUtf16(&dp, &sp, 4);
                } else {
                    u32 -= 0x10000;
                    dp[0] = (uint16_t) (0xD800 | ((u32 >> 10) & 0x3FF));
                    dp[1] = (uint16_t) (0xDC00 | (u32 & 0x3FF));
                    dp += 2;
                    sp += 4;
                }
            }
        } else {
            ReplaceUtf16(&dp, &sp, 1);
        }
    }
    assert(dp - dest <= (int) sn);
    return (int) (dp - dest);
}

void dv_append_from_utf8(d_Vector(wchar)* wstr, d_Slice(char) from)
{
    int dn = from.size;
    uint16_t* dest = dv_append_buffer(wstr, dn);
    int used = utf8_to_utf16(dest, dn, (uint8_t*) from.data, from.size);
    dv_erase_end(wstr, dn - used);
}

