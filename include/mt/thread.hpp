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

#include <mt/thread.h>
#include <mt/message.hpp>

namespace MT
{

template <class O>
struct RunThread {
    static int Run(O* o) {
        MT_RunEventLoop();
        delete o;
        return 0;
    }
};

class Thread
{
    MT_NOT_COPYABLE(Thread);
    MT_Thread* m_Thread;
public:
    Thread(const char* name)
        : m_Thread(MT_NewThread(name)),
          OnExit(*(Signal<int>*) MT_ThreadOnExit(m_Thread)) {
    }

    ~Thread() {
        MT_FreeThread(m_Thread);
    }

    void BeginInit() {
        MT_BeginThreadInit(m_Thread);
    }

    void EndInit() {
        MT_EndThreadInit(m_Thread);
    }

    void SetName(const char* name) {
        MT_SetThreadName(m_Thread, name);
    }

    template <class O>
    void Start(O* o) {
        MT_StartThread(m_Thread, BindInt(&RunThread<O>::Run, o));
    }

    template <class MF, class O>
    void Start(MF mf, O* o) {
        Delegate0<int> dlg = Delegate(mf, o);
        MT_StartThread(m_Thread, *(IntDelegate*) &dlg);
    }

    Signal<int>& OnExit;

private:
};

}
