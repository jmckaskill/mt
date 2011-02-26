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
#include <mt/event.h>
#include <delegate.h>

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus

namespace MT
{

class Event
{
    MT_NOT_COPYABLE(Event);
public:
    Event() : m_Reg(NULL) {}

    ~Event() {
        Unregister();
    }

    template <class MF1, class MF2, class MF3, class T>
    void SetupClientSocket(MT_Socket fd, MF1 read, MF2 write, MF3 close, T* p) {
        MT_FreeEvent(m_Reg);
        m_Reg = MT_NewClientSocketEvent(fd, BindVoid(read, p), BindVoid(write, p), BindVoid(close, p));
    }

    template <class MF, class T>
    void SetupServerSocket(MT_Socket fd, MF accept, T* p) {
        MT_FreeEvent(m_Reg);
        m_Reg = MT_NewServerSocketEvent(fd, BindVoid(accept, p));
    }

    template <class MF, class T>
    void SetupHandle(MT_Handle h, MF cb, T* p) {
        MT_FreeEvent(m_Reg);
        m_Reg = MT_NewHandleEvent(h, BindVoid(cb, p));
    }

    template <class MF, class T>
    void SetupIdle(MF cb, T* p) {
        MT_FreeEvent(m_Reg);
        m_Reg = MT_NewIdleEvent(BindVoid(cb, p));
    }

    template <class MF, class T>
    void SetupTick(MT_Time period, MF cb, T* p) {
        MT_FreeEvent(m_Reg);
        m_Reg = MT_NewTickEvent(period, BindVoid(cb, p));
    }

    void Unregister() {
        MT_FreeEvent(m_Reg);
        m_Reg = NULL;
    }

private:
    MT_Event* m_Reg;
};

}

#endif
