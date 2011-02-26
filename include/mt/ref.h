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
#include <mt/atomic.h>

#define MT_Ref2(r, hl) ((void) ((r) && MT_AtomicIncrement(&(r)->hl)))
#define MT_Deref2(r, hl, free) ((void) ((r) && MT_AtomicDecrement(&(r)->hl) == 0 && (free, 0)))

#define MT_Ref(r) MT_Ref2(r, ref)
#define MT_Deref(r, free) MT_Deref2(r, ref, free)

#ifdef __cplusplus
namespace MT
{

/* ------------------------------------------------------------------------- */

template <class T>
class Ptr
{
public:
    Ptr() : m_P(NULL) {
    }

    Ptr(T* p) : m_P(p) {
        MT_Ref(m_P);
    }

    Ptr(const Ptr<T>& r) : m_P(r.m_P) {
        MT_Ref(m_P);
    }

    ~Ptr() {
        MT_Deref(m_P, Delete(m_P));
    }

    Ptr<T>& operator=(const Ptr<T>& r) {
        reset(r.m_P);
        return *this;
    }

    void reset(T* p = NULL) {
        MT_Ref(p);
        MT_Deref(m_P, Delete(m_P));
        m_P = p;
    }

    T* get() const {
        return m_P;
    }

    T* operator->() const {
        return m_P;
    }

    operator T*() const {
        return m_P;
    }

private:
    T* m_P;
};

/* ------------------------------------------------------------------------- */

}
#endif
