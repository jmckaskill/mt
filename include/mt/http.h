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
#include <dmem/char.h>
#include <dmem/delegates.h>

DECLARE_DELEGATE_2(MT_HttpData, int, d_Slice(char), bool);
#define MT_BindHttpData(func, obj) BIND2(MT_HttpData, func, obj, d_Slice(char)*, bool*)

DECLARE_DELEGATE_1(MT_HttpRequest, void, MT_HttpData*);
#define MT_BindHttpRequest(func, obj) BIND1(MT_HttpRequest, func, obj, MT_HttpData**)

MT_API MT_Http* MT_NewHttp(MT_BufferedIO* io, MT_HttpRequest req);
MT_API void MT_FreeHttp(MT_Http* h);

MT_API d_Slice(char) MT_GetHttpHeader(MT_Http* h, d_Slice(char) key);
#define MT_GetHttpMethod(h) MT_GetHttpHeader(h, C("method"))
#define MT_GetHttpPath(h) MT_GetHttpHeader(h, C("path"))

MT_API void MT_SetHttpHeader(MT_Http* h, d_Slice(char) key, d_Slice(char) value);
MT_API void MT_SetHttpHeader2(MT_Http* h, d_Slice(char) key, int value);
MT_API void MT_SendHttpResponse(MT_Http* h, int code);
MT_API void MT_SendHttpResponse2(MT_Http* h, int code, d_Slice(char) data);

