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
#include <dmem/char.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define MT_SOCKET_INVALID INVALID_SOCKET
typedef SOCKADDR_STORAGE MT_SockaddrStorage;
typedef int MT_Socklen;

#else
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#define MT_SOCKET_INVALID -1
#define closesocket(x) close(x)
typedef struct sockaddr_storage MT_SockaddrStorage;
typedef socklen_t MT_Socklen;

#endif

struct MT_Sockaddr {
    MT_SockaddrStorage  sa;
    MT_Socklen          len;
};

#define MT_SOCKET_REUSEADDR 1
#define MT_SOCKET_LISTEN    2

DVECTOR_INIT(MT_Socket, MT_Socket);

/* Url is a string of the form <hostname>:<port> */
MT_API MT_Socket MT_ConnectUDP(d_Slice(char) url, int flags);
MT_API MT_Socket MT_ConnectTCP(d_Slice(char) url, int flags);
MT_API void MT_BindUDP(d_Slice(char) url, d_Vector(MT_Socket)* ret, int flags);
MT_API void MT_BindTCP(d_Slice(char) url, d_Vector(MT_Socket)* ret, int flags);

/* Loopback UDP sockets can give spurious ECONNREFUSED errors on linux boxes
 */
MT_API int MT_UDPSendTo2(MT_Socket sfd, d_Slice(char) data, const struct sockaddr* sa, MT_Socklen salen);
MT_API int MT_UDPSendTo(MT_Socket sfd, d_Slice(char) data, const MT_Sockaddr* sa);
MT_API MT_Socket MT_AcceptTCP(MT_Socket sfd, MT_Sockaddr* sa);

#define MT_LOOKUP_HOST 1

MT_API void MT_SockaddrUrl(d_Vector(char)* out, const MT_Sockaddr* sa, int flags);
MT_API void MT_SockaddrUrl2(d_Vector(char)* out, const struct sockaddr* sa, size_t salen, int flags);
MT_API void MT_SocketUrl(d_Vector(char)* out, MT_Socket sock, int flags);
MT_API void MT_PeerUrl(d_Vector(char)* out, MT_Socket sock, int flags);
MT_API int64_t MT_SocketPeerPid(MT_Socket sock);



