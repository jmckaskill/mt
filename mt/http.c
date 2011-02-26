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

#include <mt/http.h>
#include <mt/bio.h>
#include <mt/filesystem.h>
#include <dmem/hash.h>
#include <dmem/xml.h>
#include <stdio.h>

DHASH_INIT_STR(char_slice, d_Slice(char));
static int Parse(MT_Http* s, d_Slice(char) str);

struct MT_Http {
    MT_HttpData on_data;
    MT_HttpRequest on_request;
    d_StringHash(char_slice) headers;
    d_Vector(char) header_data;
    bool headers_parsed;
    int content_left;
    d_Vector(char) tx_headers;
    d_Vector(char) tx_data;
    d_Vector(char) tx_code;
    d_Vector(char) rx_path_decoded;
    d_Vector(char) rx_path_normalised;
    MT_BufferedIO* io;
};

static void Reset(MT_Http* h)
{
    dv_clear(&h->tx_headers);
    dv_clear(&h->tx_data);
    dv_clear(&h->header_data);
    dh_clear(&h->headers);
    h->content_left = -1;
    h->headers_parsed = false;
    h->on_data.func = NULL;
}

MT_Http* MT_NewHttp(MT_BufferedIO* io, MT_HttpRequest req)
{
    MT_Http* s = NEW(MT_Http);
    s->on_request = req;
    s->io = io;
    io->on_rx = BindSlice(&Parse, s);
    Reset(s);
    return s;
}

void MT_FreeHttp(MT_Http* s)
{
    if (s) {
        dv_free(s->header_data);
        dv_free(s->tx_headers);
        dv_free(s->tx_data);
        dv_free(s->tx_code);
        dv_free(s->rx_path_decoded);
        dv_free(s->rx_path_normalised);
        dh_free(&s->headers);
        free(s);
    }
}

d_Slice(char) MT_GetHttpHeader(MT_Http* h, d_Slice(char) key)
{
    d_Slice(char) ret = DV_INIT;
    dhs_get(&h->headers, key, &ret);
    return ret;
}

static int Parse(MT_Http* s, d_Slice(char) str)
{
    int used = 0;
    int on_data_used;

    fprintf(stderr, "Rx:\n%.*s\n", DV_PRI(str));

    if (!s->headers_parsed) {

        /* Make sure that we have all the header data */
        for (;;) {
            d_Slice(char) line;

            if (!dv_has_next_line2(str, &line, &used)) {
                dv_append(&s->header_data, str);
                return str.size;

            } else if (line.size > 0) {
                continue;

            } else if (line.data == str.data && !dv_ends_with(s->header_data, C("\n"))) {
                continue;
            }

            dv_append(&s->header_data, dv_left(str, used));
            break;
        }

        /* Parse the header data */

        {
            int parse_used = 0;
            d_Slice(char) line, method, path, version;
            dv_tokenizer tok;

            if (!dv_has_next_line2(s->header_data, &line, &parse_used)) {
                return -1;
            }

            dv_load_token(&tok, line, " ");
            method = dv_next_token(&tok);
            path = dv_next_token(&tok);
            version = dv_next_token(&tok);

            if (!method.size || !path.size || !version.size) {
                return -1;
            }

            dv_clear(&s->rx_path_decoded);
            dv_append_url_decoded(&s->rx_path_decoded, path);

            dv_clear(&s->rx_path_normalised);
            dv_append_normalised_path(&s->rx_path_normalised, s->rx_path_decoded);

            dhs_set(&s->headers, C("method"), method);
            dhs_set(&s->headers, C("path"), s->rx_path_normalised);
            dhs_set(&s->headers, C("version"), version);

            while (dv_has_next_line2(s->header_data, &line, &parse_used) && line.size) {
                d_Slice(char) key, value;
                int colon, idx;

                colon = dv_find_char(line, ':');
                if (colon < 0) {
                    return -1;
                }

                key = dv_left(line, colon);
                value = dv_strip_whitespace(dv_right(line, colon + 1));

                if (!dhs_add(&s->headers, key, &idx)) {
                    return -1;
                }

                s->headers.vals[idx] = value;

                if (dv_equals(key, C("Content-Length"))) {
                    s->content_left = dv_to_integer(value, 10, -1);
                }
            }
        }

        CALL_DELEGATE_1(s->on_request, &s->on_data);
    }

    s->headers_parsed = true;

    if (s->content_left <= 0) {
        Reset(s);
        return used;
    }

    if (s->on_data.func == NULL) {
        return -1;
    }

    str = dv_right(str, used);

    if (str.size >= s->content_left) {
        str = dv_left(str, s->content_left);
        on_data_used = CALL_DELEGATE_2(s->on_data, str, true);
        Reset(s);

        if (on_data_used < 0) {
            return on_data_used;
        } else {
            used += str.size;
            return used;
        }

    } else {
        on_data_used = CALL_DELEGATE_2(s->on_data, str, false);

        if (on_data_used < 0) {
            return on_data_used;
        } else {
            s->content_left -= on_data_used;
            used += on_data_used;
            return used;
        }
    }
}

void MT_SendHttpResponse(MT_Http* h, int code)
{
    MT_SendHttpResponse2(h, code, C(""));
}

void MT_SendHttpResponse2(MT_Http* h, int code, d_Slice(char) data)
{
    dv_clear(&h->tx_code);
    dv_print(&h->tx_code, "HTTP/1.1 %d\r\n", code);
    MT_SetHttpHeader2(h, C("Content-Length"), data.size);
    MT_SendData(h->io, h->tx_code);
    MT_SendData(h->io, h->tx_headers);
    MT_SendData(h->io, C("\r\n"));
    MT_SendData(h->io, data);

    fprintf(stderr, "Tx:\n%.*s%.*s\r\n%.*s\n\n",
            DV_PRI(h->tx_code),
            DV_PRI(h->tx_headers),
            DV_PRI(data));

    dv_clear(&h->tx_headers);
}

void MT_SetHttpHeader(MT_Http* h, d_Slice(char) key, d_Slice(char) value)
{
    dv_append(&h->tx_headers, key);
    dv_append(&h->tx_headers, C(": "));
    dv_append_xml_encoded(&h->tx_headers, value);
    dv_append(&h->tx_headers, C("\r\n"));
}

void MT_SetHttpHeader2(MT_Http* h, d_Slice(char) key, int value)
{
    dv_append(&h->tx_headers, key);
    dv_print(&h->tx_headers, ": %d\r\n", value);
}

