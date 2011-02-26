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

#define _POSIX_SOURCE

#include "mt-internal.h"
#include <mt/bio.h>
#include <mt/event.h>
#include <mt/socket.h>
#include <mt/time.h>
#include <dmem/file.h>
#include <assert.h>
#include <stdbool.h>

#ifndef _WIN32
#include <errno.h>
#include <signal.h>
#endif

#ifdef MT_USE_SSL
#include <openssl/err.h>
#endif

#define BUFSZ (16 * 1024)

typedef struct MTI_BufferedIO MTI_BufferedIO;

struct MTI_BufferedIO {
    MT_BufferedIO           h;

    VoidDelegate            free;

    d_Vector(char)          tx_buf;
    d_Vector(char)          rx_buf;
    d_Vector(char)          log;
    d_Vector(char)          keepalive_data;

    MT_Event*               sock_reg;
    MT_Event*               idle_reg;
    MT_Event*               keepalive_reg;

    MT_Socket               sock;
    bool                    close_socket_on_free;
    bool                    is_server;

#ifdef MT_USE_SSL
    SSL*                    ssl;
    bool                    flush_after_read;
    bool                    in_init;
#endif
};

static void Socket_Free(MTI_BufferedIO* s);
static void Socket_ReadyRead(MTI_BufferedIO* s);
static void Socket_OnIdle(MTI_BufferedIO* s);
static void Socket_Send(MTI_BufferedIO* s);
static void KeepaliveTimeout(MTI_BufferedIO* s);

static void SSL_ReadyRead(MTI_BufferedIO* s);
static void SSL_OnError(MTI_BufferedIO* s, int ret);
static void SSL_Free(MTI_BufferedIO* s);
static void SSL_OnIdle(MTI_BufferedIO* s);
static void SSL_Send(MTI_BufferedIO* s);
static void SSL_MsgCallback(int write_p, int version, int content_type, const void* buf, size_t len, SSL* ssl, void* arg);

/* ------------------------------------------------------------------------- */

void MT_CloseBufferedIO(MT_BufferedIO* io)
{
    MTI_BufferedIO* s = (MTI_BufferedIO*) io;
    MT_LOG("IO close %.*s\n", DV_PRI(s->log));
    CALL_DELEGATE_0(io->on_close);
}

/* ------------------------------------------------------------------------- */

bool MT_SendFile(MT_BufferedIO* io, d_Slice(char) filename)
{
    MTI_BufferedIO* s = (MTI_BufferedIO*) io;

    MT_LOG("IO TX %.*s file %.*s\n", DV_PRI(s->log), DV_PRI(filename));

    if (dv_append_file(&s->tx_buf, filename)) {
        MT_EnableEvent(s->idle_reg, MT_EVENT_IDLE);
        return true;

    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------- */

int MT_SendData(MT_BufferedIO* io, d_Slice(char) data)
{
    MTI_BufferedIO* s = (MTI_BufferedIO*) io;

    if (MT_LOG_ENABLED) {
        d_Vector(char) dbg = DV_INIT;
        dv_append_hex_dump(&dbg, data, MT_LOG_COLOR);
        MT_LOG("IO TX %.*s\n%.*s\n", DV_PRI(s->log), DV_PRI(dbg));
        dv_free(dbg);
    }

    dv_append(&s->tx_buf, data);
    MT_EnableEvent(s->idle_reg, MT_EVENT_IDLE);
    return data.size;
}

/* ------------------------------------------------------------------------- */

void MT_FreeBufferedIO(MT_BufferedIO* io)
{
    MTI_BufferedIO* s = (MTI_BufferedIO*) io;
    CALL_DELEGATE_0(s->free);
}

/* ------------------------------------------------------------------------- */

MT_BufferedIO* MT_NewBufferedSocket(MT_Socket sock, int flags)
{
    MTI_BufferedIO* s = NEW(MTI_BufferedIO);

    s->free = BindVoid(&Socket_Free, s);

    s->close_socket_on_free = (flags & MT_CLOSE_SOCKET_ON_FREE) != 0;
    s->is_server = (flags & MT_SERVER_BIO) != 0;
    s->sock = sock;

    s->sock_reg = MT_NewClientSocketEvent(
            sock,
            BindVoid(&Socket_ReadyRead, s),
            BindVoid(&Socket_OnIdle, s),
            BindVoid(&MT_CloseBufferedIO, &s->h));

    s->idle_reg = MT_NewIdleEvent(
            BindVoid(&Socket_OnIdle, s));

    if (MT_LOG_ENABLED) {
        MT_PeerUrl(&s->log, sock, MT_LOOKUP_HOST);
    }

#ifndef _WIN32
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa, NULL);
    }
#endif

    MT_DisableEvent(s->idle_reg, MT_EVENT_IDLE);
    MT_DisableEvent(s->sock_reg, MT_EVENT_WRITE);

    return &s->h;
}

/* ------------------------------------------------------------------------- */

static void Socket_Free(MTI_BufferedIO* s)
{
    /* Try and flush out any remaining data */
    Socket_OnIdle(s);

    MT_FreeEvent(s->idle_reg);
    MT_FreeEvent(s->sock_reg);
    MT_FreeEvent(s->keepalive_reg);

    if (s->close_socket_on_free) {
        closesocket(s->sock);
    }

    dv_free(s->tx_buf);
    dv_free(s->rx_buf);
    dv_free(s->log);
    dv_free(s->keepalive_data);

    free(s);
}

/* ------------------------------------------------------------------------- */

static void Socket_ReadyRead(MTI_BufferedIO* s)
{
    char* dest;
    int got = BUFSZ;
    int used;

    while (got == BUFSZ) {
        dest = dv_append_buffer(&s->rx_buf, BUFSZ);
        got = recv(s->sock, dest, BUFSZ, 0);
        dv_erase_end(&s->rx_buf, (got >= 0 ? BUFSZ - got : BUFSZ));

#ifndef _WIN32
        /* Force us to go around again */
        if (got < 0 && errno == EINTR) {
            got = BUFSZ;
        }
#endif
    }

    if (got == 0) {
        MT_CloseBufferedIO(&s->h);
        return;
    } 
    
#ifdef _WIN32
    if (got < 0 && GetLastError() != WSAEWOULDBLOCK) 
#else
    if (got < 0 && errno != EAGAIN) 
#endif
    {
        MT_CloseBufferedIO(&s->h);
        return;
    }

    MT_ResetEvent(s->keepalive_reg);

    if (MT_LOG_ENABLED) {
        d_Vector(char) dbg = DV_INIT;
        dv_append_hex_dump(&dbg, dv_right(s->rx_buf, -got), MT_LOG_COLOR);
        MT_LOG("IO RX %.*s\n%.*s\n", DV_PRI(s->log), DV_PRI(dbg));
        dv_free(dbg);
    }

    used = CALL_DELEGATE_1(s->h.on_rx, s->rx_buf);

    if (used < 0) {
        MT_CloseBufferedIO(&s->h);
    } else {
        dv_erase(&s->rx_buf, 0, used);
    }
}

/* ------------------------------------------------------------------------- */

static void Socket_OnIdle(MTI_BufferedIO* s)
{
    int written = 0;

    MT_DisableEvent(s->idle_reg, MT_EVENT_IDLE);

    if (s->tx_buf.size == 0) {
        return;
    }

    MT_ResetEvent(s->keepalive_reg);

#ifndef _WIN32
    do {
#endif
        int ret = (int) send(s->sock, s->tx_buf.data + written, s->tx_buf.size - written, 0);
        written += (ret >= 0) ? ret : 0;

#ifndef _WIN32
    } while (written < s->tx_buf.size && errno == EINTR);
#endif

#if 0
    if (MT_LOG_ENABLED) {
        d_Vector(char) dbg = DV_INIT;
        dv_append_hex_dump(&dbg, dv_left(s->tx_buf, written), MT_LOG_COLOR);
        MT_LOG("IO Flush %.*s\n%.*s\n", DV_PRI(s->log), DV_PRI(dbg));
        dv_free(dbg);
    }
#endif

    dv_erase(&s->tx_buf, 0, written);

    if (s->tx_buf.size > 0) {
        MT_EnableEvent(s->sock_reg, MT_EVENT_WRITE);
    } else {
        MT_DisableEvent(s->sock_reg, MT_EVENT_WRITE);
    }
}

/* ------------------------------------------------------------------------- */

void MT_SetupKeepalive(MT_BufferedIO* io, MT_Time timeout, d_Slice(char) data)
{
    MTI_BufferedIO* s = (MTI_BufferedIO*) io;
    dv_set(&s->keepalive_data, data);
    MT_FreeEvent(s->keepalive_reg);

    if (MT_TIME_ISVALID(timeout) && timeout > 0) {
        s->keepalive_reg = MT_NewTickEvent(timeout, BindVoid(&KeepaliveTimeout, s));
    } else {
        s->keepalive_reg = NULL;
    }
}

/* ------------------------------------------------------------------------- */

static void KeepaliveTimeout(MTI_BufferedIO* s)
{
    MT_SendData(&s->h, s->keepalive_data);
}






#ifdef MT_USE_SSL

/* ------------------------------------------------------------------------- */

static int PrintErrors(const char* str, size_t len, void* u)
{
    (void) u;
#if defined _WIN32 && defined _DEBUG && !defined _WIN32_WCE
    OutputDebugStringA(str);
#endif
    fwrite(str, 1, len, stderr);
    return len;
}

bool MT_EnableTLS(MT_BufferedIO* io, SSL_CTX* ctx)
{
    MTI_BufferedIO* s = (MTI_BufferedIO*) io;
    int ret;

    if (s->ssl) {
        return true;
    }

    /* Try and flush out any remaining data */
    Socket_OnIdle(s);
    dv_clear(&s->tx_buf);

    if (ctx) {
        s->ssl = SSL_new(ctx);
    } else {
        const SSL_METHOD* meth = SSLv3_client_method();
        ctx = SSL_CTX_new((SSL_METHOD*) meth);
        s->ssl = SSL_new(ctx);
        SSL_CTX_free(ctx);
    }

    if (s->ssl == NULL) {
        ERR_print_errors_cb(&PrintErrors, NULL);
        return false;
    }

    if (MT_LOG_ENABLED) {
        SSL_set_msg_callback(s->ssl, &SSL_MsgCallback);
        SSL_set_msg_callback_arg(s->ssl, s);
    }

    SSL_set_fd(s->ssl, s->sock);
    SSL_set_verify(s->ssl, SSL_VERIFY_NONE, NULL);

    if (s->is_server) {
        SSL_set_accept_state(s->ssl);
    } else {
        SSL_set_connect_state(s->ssl);
    }

    MT_FreeEvent(s->idle_reg);
    MT_FreeEvent(s->sock_reg);

    s->free = BindVoid(&SSL_Free, s);
    s->flush_after_read = false;

    s->sock_reg = MT_NewClientSocketEvent(
            s->sock,
            BindVoid(&SSL_ReadyRead, s),
            BindVoid(&SSL_OnIdle, s),
            BindVoid(&MT_CloseBufferedIO, &s->h));

    s->idle_reg = MT_NewIdleEvent(
            BindVoid(&SSL_OnIdle, s));

    MT_DisableEvent(s->idle_reg, MT_EVENT_IDLE);
    MT_DisableEvent(s->sock_reg, MT_EVENT_WRITE);

    ret = SSL_do_handshake(s->ssl);
    if (ret <= 0) {
        s->in_init = true;
        SSL_OnError(s, ret);
    } else {
        s->in_init = false;
    }

    return true;
}

/* ------------------------------------------------------------------------- */

void MT_DisableTLS(MT_BufferedIO* io)
{
    MTI_BufferedIO* s = (MTI_BufferedIO*) io;

    if (s->ssl) {
        /* Try and flush out any remaining data */
        SSL_OnIdle(s);
        dv_clear(&s->tx_buf);

        SSL_shutdown(s->ssl);
        SSL_free(s->ssl);
        s->ssl = NULL;
        s->flush_after_read = false;
        s->free = BindVoid(&Socket_Free, s);

        MT_FreeEvent(s->idle_reg);
        MT_FreeEvent(s->sock_reg);

        s->sock_reg = MT_NewClientSocketEvent(
                s->sock,
                BindVoid(&Socket_ReadyRead, s),
                BindVoid(&Socket_OnIdle, s),
                BindVoid(&MT_CloseBufferedIO, &s->h));

        s->idle_reg = MT_NewIdleEvent(
                BindVoid(&Socket_OnIdle, s));

        MT_DisableEvent(s->idle_reg, MT_EVENT_IDLE);
        MT_DisableEvent(s->sock_reg, MT_EVENT_WRITE);
    }
}

/* ------------------------------------------------------------------------- */

MT_BufferedIO* MT_NewBufferedSSL(MT_Socket sock, SSL_CTX* ctx, int flags)
{
    MT_BufferedIO* io = MT_NewBufferedSocket(sock, flags);

    if (MT_EnableTLS(io, ctx)) {
        return io;
    } else {
        MT_FreeBufferedIO(io);
        return NULL;
    }
}

/* ------------------------------------------------------------------------- */

static void SSL_Free(MTI_BufferedIO* s)
{
    MT_DisableTLS(&s->h);
    Socket_Free(s);
}

/* ------------------------------------------------------------------------- */

static void SSL_OnError(MTI_BufferedIO* s, int ret)
{
    switch (SSL_get_error(s->ssl, ret)) {
        case SSL_ERROR_ZERO_RETURN:
            /* Connection has been closed cleanly */
            MT_CloseBufferedIO(&s->h);
            break;

        case SSL_ERROR_WANT_WRITE:
            /* Wait for us to be called again */
            MT_EnableEvent(s->sock_reg, MT_EVENT_WRITE);
            break;

        case SSL_ERROR_WANT_READ:
            /* Wait for ReadyRead to be called */
            s->flush_after_read = true;
            if (s->in_init && !SSL_in_init(s->ssl)) {
                s->in_init = false;
                SSL_OnIdle(s);
            }
            break;

        case SSL_ERROR_SSL:
            ERR_print_errors_cb(&PrintErrors, NULL);
            MT_CloseBufferedIO(&s->h);
            break;

        case SSL_ERROR_SYSCALL:
            ERR_print_errors_cb(&PrintErrors, NULL);
            MT_CloseBufferedIO(&s->h);
            break;

        case SSL_ERROR_WANT_X509_LOOKUP:
            /* TBD - what to do - for now fall through*/
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
        default:
            /* These shouldn't happen */
            assert(0);
            MT_CloseBufferedIO(&s->h);
            break;
    }
}

/* ------------------------------------------------------------------------- */

static void SSL_ReadyRead(MTI_BufferedIO* s)
{
    int got, used;

    size_t to_read = SSL_pending(s->ssl) + BUFSZ;
    char* dest = dv_append_buffer(&s->rx_buf, to_read);

    got = SSL_read(s->ssl, dest, to_read);

    MT_ResetEvent(s->keepalive_reg);
    dv_erase_end(&s->rx_buf, (got >= 0) ? BUFSZ - got : BUFSZ);

    if (got <= 0) {
        SSL_OnError(s, got);
        return;
    }

    if (s->flush_after_read) {
        s->flush_after_read = false;
        SSL_OnIdle(s);
    }

    if (MT_LOG_ENABLED) {
        d_Vector(char) dbg = DV_INIT;
        dv_append_hex_dump(&dbg, dv_right(s->rx_buf, -got), MT_LOG_COLOR);
        MT_LOG("IO RX %.*s\n%.*s\n", DV_PRI(s->log), DV_PRI(dbg));
        dv_free(dbg);
    }

    used = CALL_DELEGATE_1(s->h.on_rx, s->rx_buf);

    if (used < 0) {
        MT_CloseBufferedIO(&s->h);
    } else {
        dv_erase(&s->rx_buf, 0, used);
    }
}

/* ------------------------------------------------------------------------- */

static void SSL_OnIdle(MTI_BufferedIO* s)
{
    int sent;

    s->flush_after_read = false;
    MT_DisableEvent(s->idle_reg, MT_EVENT_IDLE);

    if (s->tx_buf.size == 0) {
        return;
    }

    MT_ResetEvent(s->keepalive_reg);
    sent = SSL_write(s->ssl, s->tx_buf.data, s->tx_buf.size);

    if (sent <= 0) {
        SSL_OnError(s, sent);
        return;
    }

#if 0
    if (MT_LOG_ENABLED) {
        d_Vector(char) dbg = DV_INIT;
        dv_append_hex_dump(&dbg, dv_left(s->tx_buf, sent), MT_LOG_COLOR);
        MT_LOG("IO Flush %.*s\n%.*s\n", DV_PRI(s->log), DV_PRI(dbg));
        dv_free(dbg);
    }
#endif

    dv_erase(&s->tx_buf, 0, sent);

    if (s->tx_buf.size) {
        MT_EnableEvent(s->sock_reg, MT_EVENT_WRITE);
    } else {
        MT_DisableEvent(s->sock_reg, MT_EVENT_WRITE);
    }
}

/* ------------------------------------------------------------------------- */

static void SSL_MsgCallback(int write_p, int version, int content_type, const void* buf, size_t len, SSL* ssl, void* arg)
{
    MTI_BufferedIO* s = (MTI_BufferedIO*) arg;
	const char *str_version, *str_content_type = "", *str_details1 = "", *str_details2= "";
    d_Vector(char) out = DV_INIT;

	switch (version)
    {
        case SSL2_VERSION:
            str_version = "SSL 2.0";
            break;
        case SSL3_VERSION:
            str_version = "SSL 3.0";
            break;
        case TLS1_VERSION:
            str_version = "TLS 1.0";
            break;
        case DTLS1_VERSION:
            str_version = "DTLS 1.0";
            break;
        case DTLS1_BAD_VER:
            str_version = "DTLS 1.0 (bad)";
            break;
        default:
            str_version = "???";
    }

    if (version == SSL2_VERSION)
    {
        str_details1 = "???";

        if (len > 0)
        {
            switch (((const unsigned char*)buf)[0])
            {
                case 0:
                    str_details1 = ", ERROR:";
                    str_details2 = " ???";
                    if (len >= 3)
                    {
                        unsigned err = (((const unsigned char*)buf)[1]<<8) + ((const unsigned char*)buf)[2];

                        switch (err)
                        {
                            case 0x0001:
                                str_details2 = " NO-CIPHER-ERROR";
                                break;
                            case 0x0002:
                                str_details2 = " NO-CERTIFICATE-ERROR";
                                break;
                            case 0x0004:
                                str_details2 = " BAD-CERTIFICATE-ERROR";
                                break;
                            case 0x0006:
                                str_details2 = " UNSUPPORTED-CERTIFICATE-TYPE-ERROR";
                                break;
                        }
                    }

                    break;
                case 1:
                    str_details1 = ", CLIENT-HELLO";
                    break;
                case 2:
                    str_details1 = ", CLIENT-MASTER-KEY";
                    break;
                case 3:
                    str_details1 = ", CLIENT-FINISHED";
                    break;
                case 4:
                    str_details1 = ", SERVER-HELLO";
                    break;
                case 5:
                    str_details1 = ", SERVER-VERIFY";
                    break;
                case 6:
                    str_details1 = ", SERVER-FINISHED";
                    break;
                case 7:
                    str_details1 = ", REQUEST-CERTIFICATE";
                    break;
                case 8:
                    str_details1 = ", CLIENT-CERTIFICATE";
                    break;
            }
        }
    }

    if (version == SSL3_VERSION ||
            version == TLS1_VERSION ||
            version == DTLS1_VERSION ||
            version == DTLS1_BAD_VER)
    {
        switch (content_type)
        {
            case 20:
                str_content_type = "ChangeCipherSpec";
                break;
            case 21:
                str_content_type = "Alert";
                break;
            case 22:
                str_content_type = "Handshake";
                break;
        }

        if (content_type == 21) /* Alert */
        {
            str_details1 = ", ???";

            if (len == 2)
            {
                switch (((const unsigned char*)buf)[0])
                {
                    case 1:
                        str_details1 = ", warning";
                        break;
                    case 2:
                        str_details1 = ", fatal";
                        break;
                }

                str_details2 = " ???";
                switch (((const unsigned char*)buf)[1])
                {
                    case 0:
                        str_details2 = " close_notify";
                        break;
                    case 10:
                        str_details2 = " unexpected_message";
                        break;
                    case 20:
                        str_details2 = " bad_record_mac";
                        break;
                    case 21:
                        str_details2 = " decryption_failed";
                        break;
                    case 22:
                        str_details2 = " record_overflow";
                        break;
                    case 30:
                        str_details2 = " decompression_failure";
                        break;
                    case 40:
                        str_details2 = " handshake_failure";
                        break;
                    case 42:
                        str_details2 = " bad_certificate";
                        break;
                    case 43:
                        str_details2 = " unsupported_certificate";
                        break;
                    case 44:
                        str_details2 = " certificate_revoked";
                        break;
                    case 45:
                        str_details2 = " certificate_expired";
                        break;
                    case 46:
                        str_details2 = " certificate_unknown";
                        break;
                    case 47:
                        str_details2 = " illegal_parameter";
                        break;
                    case 48:
                        str_details2 = " unknown_ca";
                        break;
                    case 49:
                        str_details2 = " access_denied";
                        break;
                    case 50:
                        str_details2 = " decode_error";
                        break;
                    case 51:
                        str_details2 = " decrypt_error";
                        break;
                    case 60:
                        str_details2 = " export_restriction";
                        break;
                    case 70:
                        str_details2 = " protocol_version";
                        break;
                    case 71:
                        str_details2 = " insufficient_security";
                        break;
                    case 80:
                        str_details2 = " internal_error";
                        break;
                    case 90:
                        str_details2 = " user_canceled";
                        break;
                    case 100:
                        str_details2 = " no_renegotiation";
                        break;
                    case 110:
                        str_details2 = " unsupported_extension";
                        break;
                    case 111:
                        str_details2 = " certificate_unobtainable";
                        break;
                    case 112:
                        str_details2 = " unrecognized_name";
                        break;
                    case 113:
                        str_details2 = " bad_certificate_status_response";
                        break;
                    case 114:
                        str_details2 = " bad_certificate_hash_value";
                        break;
                }
            }
        }

        if (content_type == 22) /* Handshake */
        {
            str_details1 = "???";

            if (len > 0)
            {
                switch (((const unsigned char*)buf)[0])
                {
                    case 0:
                        str_details1 = ", HelloRequest";
                        break;
                    case 1:
                        str_details1 = ", ClientHello";
                        break;
                    case 2:
                        str_details1 = ", ServerHello";
                        break;
                    case 3:
                        str_details1 = ", HelloVerifyRequest";
                        break;
                    case 11:
                        str_details1 = ", Certificate";
                        break;
                    case 12:
                        str_details1 = ", ServerKeyExchange";
                        break;
                    case 13:
                        str_details1 = ", CertificateRequest";
                        break;
                    case 14:
                        str_details1 = ", ServerHelloDone";
                        break;
                    case 15:
                        str_details1 = ", CertificateVerify";
                        break;
                    case 16:
                        str_details1 = ", ClientKeyExchange";
                        break;
                    case 20:
                        str_details1 = ", Finished";
                        break;
                }
            }
        }
    }

    dv_print(&out, "%s %s %.*s %s%s%s\n",
            str_version,
            write_p ? "TX" : "RX",
            DV_PRI(s->log),
            str_content_type,
            str_details1,
            str_details2);

    dv_append_hex_dump(&out, dv_char2((char*) buf, len), MT_LOG_COLOR);

    MT_LOG("%.*s\n", DV_PRI(out));
    dv_free(out);
}

#endif

