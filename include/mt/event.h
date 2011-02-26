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

/* ------------------------------------------------------------------------- */

MT_API MT_Event* MT_NewClientSocketEvent(MT_Socket fd, VoidDelegate read, VoidDelegate write, VoidDelegate close);
MT_API MT_Event* MT_NewServerSocketEvent(MT_Socket fd, VoidDelegate accept);
MT_API MT_Event* MT_NewHandleEvent(MT_Handle h, VoidDelegate cb);
MT_API MT_Event* MT_NewIdleEvent(VoidDelegate cb);
MT_API MT_Event* MT_NewTickEvent(MT_Time period, VoidDelegate cb);

/* Available flags for MT_EnableEvent */
#define MT_EVENT_HANDLE  0x01
#define MT_EVENT_READ    0x02
#define MT_EVENT_WRITE   0x04
#define MT_EVENT_CLOSE   0x08
#define MT_EVENT_ACCEPT  0x10
#define MT_EVENT_IDLE    0x20
#define MT_EVENT_TICK    0x40

/* Note these events can be either level or edge triggered so the calling code
 * should only enable the event when it needs it (eg only enable the write
 * event when a call to send failed) and process all data available when the
 * event is triggered.
 */

/* Must be called from the same thread as the event queue */
MT_API void MT_EnableEvent(MT_Event* r, int flags);
MT_API void MT_DisableEvent(MT_Event* r, int flags);
MT_API void MT_ResetEvent(MT_Event* r);
MT_API void MT_FreeEvent(MT_Event* r);
