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
#include <assert.h>

/* ------------------------------------------------------------------------- */

#ifdef DELEGATE_HAS_MFUNC
template <class R>
struct Delegate0 { 
    union {
        R (__fastcall *mfunc)(void*,int);
        R (*func)(void*);
    };
    void* obj;
    unsigned int use_mfunc : 1;
};

template <class R, class A>
struct Delegate1 { 
    union {
        R (__fastcall *mfunc)(void*,int,A);
        R (*func)(void*,A);
    };
    void* obj;
    unsigned int use_mfunc : 1;
};

#else
template <class R>
struct Delegate0 { 
    R (*func)(void*);
    void* obj;
};

template <class R, class A>
struct Delegate1 { 
    R (*func)(void*,A);
    void* obj;
};
#endif

/* ------------------------------------------------------------------------- */

namespace delegate_detail
{

template <size_t sz>
struct DecodeMemPtr;

#ifdef _MSC_VER
#include "delegate-msvc.h"
#elif __GNUC__
#include "delegate-gcc.h"
#else
#error
#endif

}

/* ------------------------------------------------------------------------- */

inline Delegate0<void> Bind(int null, void* obj)
{
    Delegate0<void> ret = NULL_DELEGATE;
    assert(null == 0);
    (void) null;
    (void) obj;
    return ret;
}

inline Delegate0<void> Bind(long null, void* obj)
{
    Delegate0<void> ret = NULL_DELEGATE;
    assert(null == 0);
    (void) null;
    (void) obj;
    return ret;
}

inline Delegate0<void> Bind(void* null, void* obj)
{
    Delegate0<void> ret = NULL_DELEGATE;
    assert(null == 0);
    (void) null;
    (void) obj;
    return ret;
}

/* ------------------------------------------------------------------------- */

template <class M, class R>
Delegate0<R> Bind(R (*f)(M*), long int null)
{
  Delegate0<R> ret;
  assert(null == 0);
  ret.obj = NULL;
  ret.func = reinterpret_cast<R (*)(void*)>(f);
#ifdef DELEGATE_HAS_MFUNC
  ret.use_mfunc = 0;
#endif
  return ret;
}

template <class O, class M, class R>
Delegate0<R> Bind(R (*f)(M*), O* o)
{
  Delegate0<R> ret;
  ret.obj = static_cast<M*>(o);
  ret.func = reinterpret_cast<R (*)(void*)>(f);
#ifdef DELEGATE_HAS_MFUNC
  ret.use_mfunc = 0;
#endif
  return ret;
}

template <class O, class M, class R>
Delegate0<R> Bind(R (M::*mf)(), O* o)
{
  typedef void* (M::*MF)();
  Delegate0<void*> d0 = delegate_detail::DecodeMemPtr<sizeof(mf)>::Convert(reinterpret_cast<MF>(mf), static_cast<M*>(o));
  Delegate0<R> ret;
  ret.obj = d0.obj;
  ret.func = reinterpret_cast<R (*)(void*)>(d0.func);
#ifdef DELEGATE_HAS_MFUNC
  ret.use_mfunc = 1;
#endif
  return ret;
}

template <class O, class M, class R>
Delegate0<R> Bind(R (M::*mf)() const, const O* o)
{
  typedef void* (M::*MF)();
  Delegate0<void*> d0 = delegate_detail::DecodeMemPtr<sizeof(mf)>::Convert(reinterpret_cast<MF>(mf), static_cast<M*>(const_cast<O*>(o)));
  Delegate0<R> ret;
  ret.obj = d0.obj;
  ret.func = reinterpret_cast<R (*)(void*)>(d0.func);
#ifdef DELEGATE_HAS_MFUNC
  ret.use_mfunc = 1;
#endif
  return ret;
}

/* ------------------------------------------------------------------------- */

template <class M, class R, class A>
Delegate1<R,A> Bind(R (*f)(M*,A), long int null)
{
  Delegate1<R,A> ret;
  assert(null == 0);
  ret.obj = NULL;
  ret.func = reinterpret_cast<R (*)(void*,A)>(f);
#ifdef DELEGATE_HAS_MFUNC
  ret.use_mfunc = 0;
#endif
  return ret;
}

template <class O, class M, class R, class A>
Delegate1<R,A> Bind(R (*f)(M*,A), O* o)
{
  Delegate1<R,A> ret;
  ret.obj = static_cast<M*>(o);
  ret.func = reinterpret_cast<R (*)(void*,A)>(f);
#ifdef DELEGATE_HAS_MFUNC
  ret.use_mfunc = 0;
#endif
  return ret;
}

template <class O, class M, class R, class A>
Delegate1<R, A> Bind(R (M::*mf)(A), O* o)
{
  typedef void* (M::*MF)();
  Delegate0<void*> d0 = delegate_detail::DecodeMemPtr<sizeof(mf)>::Convert(reinterpret_cast<MF>(mf), static_cast<M*>(o));
  Delegate1<R,A> ret;
  ret.obj = d0.obj;
  ret.func = reinterpret_cast<R (*)(void*,A)>(d0.func);
#ifdef DELEGATE_HAS_MFUNC
  ret.use_mfunc = 1;
#endif
  return ret;
}

template <class O, class M, class R, class A>
Delegate1<R, A> Bind(R (M::*mf)(A) const, const O* o)
{
  typedef void* (M::*MF)();
  Delegate0<void*> d0 = delegate_detail::DecodeMemPtr<sizeof(mf)>::Convert(reinterpret_cast<MF>(mf), static_cast<M*>(const_cast<O*>(o)));
  Delegate1<R,A> ret;
  ret.obj = d0.obj;
  ret.func = reinterpret_cast<R (*)(void*,A)>(d0.func);
#ifdef DELEGATE_HAS_MFUNC
  ret.use_mfunc = 1;
#endif
  return ret;
}

/* ------------------------------------------------------------------------- */

