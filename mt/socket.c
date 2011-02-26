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

#ifdef _WIN32
#   include <Winsock2.h>
#   include <WS2tcpip.h>
#   include <windows.h>
#   include <IPHlpApi.h>
#else
#   define _POSIX_SOURCE
#   define _GNU_SOURCE
#   include <netdb.h>
#endif

#include "mt-internal.h"
#include <mt/socket.h>
#include <dmem/char.h>
#include <errno.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

static MT_Socket Connect(int socktype, d_Slice(char) url, int flags)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL, *rp;
    char *hostname, *port;
    d_Vector(char) urlcopy = DV_INIT;
    MT_Socket ret = MT_SOCKET_INVALID;

#ifdef _WIN32
    WSADATA wsadata;

    if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
        goto end;
    }

#endif

    (void) flags;

    if (url.size == 0) {
        return MT_SOCKET_INVALID;
    }

    dv_set(&urlcopy, url);
    hostname = (char*) urlcopy.data;

    port = strrchr(hostname, ':');
    if (port == NULL) {
        goto end;
    }

    *port = '\0';
    port++;

    /* Strip the square brackets from around the hostname if there are any */
    if (port[-2] == ']' && hostname[0] == '[') {
        hostname++;
        port[-2] = '\0';
    }

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = socktype;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if (getaddrinfo(hostname, port, &hints, &result) != 0) {
        goto end;
    }

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        MT_Socket sfd = socket(rp->ai_family,
#ifdef SOCK_CLOEXEC
                               rp->ai_socktype | SOCK_CLOEXEC,
#else
                               rp->ai_socktype,
#endif
                               rp->ai_protocol);

        if (sfd == MT_SOCKET_INVALID) {
            continue;
        }

        if (connect(sfd, rp->ai_addr, (int) rp->ai_addrlen)) {
            closesocket(sfd);
            continue;
        }

        ret = sfd;
        break;
    }

end:
    freeaddrinfo(result);
    dv_free(urlcopy);
    return ret;
}

/* ------------------------------------------------------------------------- */

MT_Socket MT_ConnectUDP(d_Slice(char) url, int flags)
{
    return Connect(SOCK_DGRAM, url, flags);
}

MT_Socket MT_ConnectTCP(d_Slice(char) url, int flags)
{
    return Connect(SOCK_STREAM, url, flags);
}

/* ------------------------------------------------------------------------- */

static void Bind(
    int socktype,
    d_Slice(char) url,
    d_Vector(MT_Socket)* ret,
    int flags)
{
    struct addrinfo hints;
    struct addrinfo* result = NULL, *rp;
    char *hostname, *port;
    d_Vector(char) urlcopy = DV_INIT;

#ifdef _WIN32
    WSADATA wsadata;

    if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
        goto end;
    }

#endif

    if (url.size == 0) {
        return;
    }

    dv_set(&urlcopy, url);
    hostname = urlcopy.data;

    port = strrchr(hostname, ':');
    if (port == NULL) {
        goto end;
    }

    *port = '\0';
    port++;

    /* Strip the square brackets from around the hostname if there are any */
    if (port[-2] == ']' && hostname[0] == '[') {
        hostname++;
        port[-2] = '\0';
    }

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = socktype;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    if (getaddrinfo(hostname, port, &hints, &result) != 0) {
        goto end;
    }

    /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        long reuse = 1;

        MT_Socket sfd = socket(rp->ai_family,
#ifdef SOCK_CLOEXEC
                               rp->ai_socktype | SOCK_CLOEXEC,
#else
                               rp->ai_socktype,
#endif
                               rp->ai_protocol);

        if (sfd == MT_SOCKET_INVALID) {
            continue;
        }

        if ((flags & MT_SOCKET_REUSEADDR) && setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse, sizeof(reuse))) {
            closesocket(sfd);
            continue;
        }

        if (bind(sfd, rp->ai_addr, (int) rp->ai_addrlen)) {
            closesocket(sfd);
            continue;
        }

        if ((flags & MT_SOCKET_LISTEN) && listen(sfd, SOMAXCONN)) {
            closesocket(sfd);
            continue;
        }

        dv_append2(ret, &sfd, 1);
    }

end:
    freeaddrinfo(result);
    dv_free(urlcopy);
}

/* ------------------------------------------------------------------------- */

void MT_BindUDP(d_Slice(char) url, d_Vector(MT_Socket)* ret, int flags)
{
    Bind(SOCK_DGRAM, url, ret, flags);
}

void MT_BindTCP(d_Slice(char) url, d_Vector(MT_Socket)* ret, int flags)
{
    Bind(SOCK_STREAM, url, ret, flags);
}

/* ------------------------------------------------------------------------- */

MT_Socket MT_AcceptTCP(MT_Socket sfd, MT_Sockaddr* sa)
{
    MT_Socket ret;
    struct sockaddr* sa2 = NULL;
    socklen_t* salen = NULL;

    if (sa) {
        sa2 = (struct sockaddr*) &sa->sa;
        salen = &sa->len;
        sa->len = sizeof(MT_Sockaddr);
    }

#if defined __linux__
    ret = accept4(sfd, sa2, salen, SOCK_CLOEXEC);

#elif defined FD_CLOEXEC
    ret = accept(sfd, sa2, salen);
    if (ret != MT_SOCK_INVALID) {
        fcntl(ret, F_SETFD, FD_CLOEXEC);
    }

#else
    ret = accept(sfd, sa2, salen);
#endif

    return ret;
}

/* ------------------------------------------------------------------------- */

void MT_SockaddrUrl2(d_Vector(char)* out, const struct sockaddr* sa, size_t salen, int flags)
{
    char host[128];
    char port[128];
    int err;
    int niflags = NI_NUMERICSERV;

    if ((flags & MT_LOOKUP_HOST) == 0) {
        niflags |= NI_NUMERICHOST;
    }

    err = getnameinfo(
              sa,
              (int) salen,
              host,
              128,
              port,
              128,
              niflags);

    if (err) {
        return;
    }

    host[127] = '\0';
    port[127] = '\0';

    if (sa->sa_family == AF_INET6) {
        dv_append(out, C("["));
    }

    dv_append(out, dv_char(host));

    if (sa->sa_family == AF_INET6) {
        dv_append(out, C("]:"));
    } else {
        dv_append(out, C(":"));
    }

    dv_append(out, dv_char(port));
}

/* ------------------------------------------------------------------------- */

void MT_SockaddrUrl(d_Vector(char)* out, const MT_Sockaddr* sa, int flags)
{
    MT_SockaddrUrl2(out, (struct sockaddr*) &sa->sa, sa->len, flags);
}

/* ------------------------------------------------------------------------- */

void MT_SocketUrl(d_Vector(char)* out, MT_Socket sock, int flags)
{
    MT_Sockaddr sa;
    sa.len = sizeof(sa.sa);

    if (getsockname(sock, (struct sockaddr*) &sa.sa, &sa.len)) {
        return;
    }

    MT_SockaddrUrl(out, &sa, flags);
}

/* ------------------------------------------------------------------------- */

void MT_PeerUrl(d_Vector(char)* out, MT_Socket sock, int flags)
{
    MT_Sockaddr sa;
    sa.len = sizeof(sa.sa);

    if (getpeername(sock, (struct sockaddr*) &sa.sa, &sa.len)) {
        return;
    }

    MT_SockaddrUrl(out, &sa, flags);
}

/* ------------------------------------------------------------------------- */

int MT_UDPSendTo2(MT_Socket sfd, d_Slice(char) buf, const struct sockaddr* sa, MT_Socklen salen)
{
#ifdef _WIN32

    if (sa) {
        return sendto(sfd, buf.data, buf.size, 0, sa, (int) salen);
    } else {
        return send(sfd, buf.data, buf.size, 0);
    }

#else
    int i;

    for (i = 0; i < 4; i++) {
        int sent;

        if (sa) {
            sent = sendto(sfd, buf.data, buf.size, 0, sa, salen);
        } else {
            sent = send(sfd, buf.data, buf.size, 0);
        }

        if (sent >= 0 || errno != ECONNREFUSED) {
            return sent;
        }
    }

    return -1;
#endif
}

int MT_UDPSendTo(MT_Socket sfd, d_Slice(char) buf, const MT_Sockaddr* sa)
{
    return MT_UDPSendTo2(sfd, buf, (const struct sockaddr*)(sa ? &sa->sa : NULL), sa ? sa->len : 0);
}

/* ------------------------------------------------------------------------- */

#if defined _WIN32 && _MSC_VER + 0 >= 1500
typedef DWORD (WINAPI *GetExtendedTcpTable_t)(PVOID pTcpTable, PDWORD pdwSize, BOOL bOrder, ULONG ulAf, TCP_TABLE_CLASS TableClass, ULONG Reserved);

int64_t MT_SocketPeerPid(MT_Socket sock)
{
    void* table = NULL;
    MT_Sockaddr sa;
    DWORD size = 0;
    int tries = 3;
    int64_t pid = -1;
    HMODULE iphlpapi = NULL;
    GetExtendedTcpTable_t GetExtendedTcpTable;

    sa.len = sizeof(sa.sa);

    if (getpeername(sock, (struct sockaddr*) &sa.sa, &sa.len)) {
        goto end;
    }

    iphlpapi = LoadLibraryA("Iphlpapi.dll");
    if (iphlpapi == NULL) {
        goto end;
    }

    GetExtendedTcpTable = (GetExtendedTcpTable_t) GetProcAddress(iphlpapi, "GetExtendedTcpTable");
    if (GetExtendedTcpTable == NULL) {
        goto end;
    }

    for (;;) {

        DWORD ret = GetExtendedTcpTable(table, &size, FALSE, sa.sa.ss_family, TCP_TABLE_OWNER_PID_CONNECTIONS, 0);

        if (table && ret == NO_ERROR) {
            break;
        } else if (ret != NO_ERROR && ret != ERROR_INSUFFICIENT_BUFFER) {
            goto end;
        }

        if (--tries == 0) {
            goto end;
        }

        table = realloc(table, size);
    }

    if (sa.sa.ss_family == AF_INET) {
        MIB_TCPTABLE_OWNER_PID* table4 = (MIB_TCPTABLE_OWNER_PID*) table;
        DWORD i;

        for (i = 0; i < table4->dwNumEntries; i++) {
            MIB_TCPROW_OWNER_PID* e = &table4->table[i];
            struct sockaddr_in* sa4 = (struct sockaddr_in*) &sa.sa;

            if (memcmp(&e->dwLocalAddr, &sa4->sin_addr.s_addr, sizeof(sa4->sin_addr.s_addr)) == 0
                && e->dwLocalPort == sa4->sin_port) 
            {
                pid = e->dwOwningPid;
                break;
            }
        }

    } else if (sa.sa.ss_family == AF_INET6) {
        MIB_TCP6TABLE_OWNER_PID* table6 = (MIB_TCP6TABLE_OWNER_PID*) table;
        DWORD i;

        for (i = 0; i < table6->dwNumEntries; i++) {
            MIB_TCP6ROW_OWNER_PID* e = &table6->table[i];
            struct sockaddr_in6* sa6 = (struct sockaddr_in6*) &sa.sa;

            if (memcmp(e->ucLocalAddr, sa6->sin6_addr.s6_addr, sizeof(sa6->sin6_addr.s6_addr)) == 0
                && e->dwLocalPort == sa6->sin6_port) 
            {
                pid = e->dwOwningPid;
                break;
            }
        }
    }

end:
    FreeLibrary(iphlpapi);
    free(table);
    return pid;
}

#else
int64_t MT_SocketPeerPid(MT_Socket sock)
{
    (void) sock;
    return -1;
}

#endif
