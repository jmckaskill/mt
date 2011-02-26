/* vim: ts=4 sw=4 sts=4 et tw=78
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

#include <mt/common.h>
#include <mt/ref.h>
#include <dmem/char.h>
#include <dmem/delegates.h>

#if !defined MT_USE_SSL && !defined _WIN32
#define MT_USE_SSL
#endif

#ifdef MT_USE_SSL
#include <openssl/ssl.h>
#endif

struct MT_BufferedIO {
    VoidDelegate    on_close;
    SliceDelegate   on_rx;
};

#define MT_CLOSE_SOCKET_ON_FREE   0x01
#define MT_SERVER_BIO             0x02

MT_API void MT_CloseBufferedIO(MT_BufferedIO* io);
MT_API void MT_FreeBufferedIO(MT_BufferedIO* io);
MT_API bool MT_SendFile(MT_BufferedIO* io, d_Slice(char) filename);
MT_API int MT_SendData(MT_BufferedIO* io, d_Slice(char) data);
MT_API char* MT_GetSendBuffer(MT_BufferedIO* io, int size);

MT_API MT_BufferedIO* MT_NewBufferedSocket(MT_Socket sock, int flags);
MT_API MT_BufferedIO* MT_NewBufferedSSL(MT_Socket sock, SSL_CTX* ctx, int flags);
MT_API void MT_SetupKeepalive(MT_BufferedIO* io, MT_Time timeout, d_Slice(char) data);
MT_API bool MT_EnableTLS(MT_BufferedIO* io, SSL_CTX* ctx);
MT_API void MT_DisableTLS(MT_BufferedIO* io);



