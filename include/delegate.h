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

/* ------------------------------------------------------------------------- */

#if defined _MSC_VER && defined _M_IX86 
#define DELEGATE_HAS_MFUNC
#endif

/* ------------------------------------------------------------------------- */

#ifdef DELEGATE_HAS_MFUNC
#define NULL_DELEGATE {NULL, NULL, 0}
#define CALL_DELEGATE_0(dlg) ( ((dlg).use_mfunc) ? (dlg).mfunc((dlg).obj,0) : (dlg).func((dlg).obj) )
#define CALL_DELEGATE_1(dlg,a) ( ((dlg).use_mfunc) ? (dlg).mfunc((dlg).obj,0,a) : (dlg).func((dlg).obj,a) )
#define CALL_DELEGATE_2(dlg,a,b) ( ((dlg).use_mfunc) ? (dlg).mfunc((dlg).obj,0,a,b) : (dlg).func((dlg).obj,a,b) )

#else
#define NULL_DELEGATE {NULL, NULL}
#define CALL_DELEGATE_0(dlg) ((dlg).func((dlg).obj))
#define CALL_DELEGATE_1(dlg,a) ((dlg).func((dlg).obj,a))
#define CALL_DELEGATE_2(dlg,a,b) ((dlg).func((dlg).obj,a,b))

#endif

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
#include "delegate-cpp.h"
#endif

/* ------------------------------------------------------------------------- */
#if defined __cplusplus && defined DELEGATE_HAS_MFUNC

#define DECLARE_DELEGATE_0(name, R)                                         \
    typedef R (*name##_cb)(void*);                                          \
    struct name {                                                           \
        union {                                                             \
            R (__fastcall* mfunc)(void*,int);                               \
            R (*func)(void*);                                               \
        };                                                                  \
        void* obj;                                                          \
        unsigned int use_mfunc : 1;                                         \
    };                                                                      \
    template <class F, class O>                                             \
    static name Bind_##name(F func, O obj) {                                \
        Delegate0<R> cpp = Bind(func, obj);                                 \
        struct name ret;                                                    \
        ret.func = cpp.func;                                                \
        ret.obj = cpp.obj;                                                  \
        ret.use_mfunc = cpp.use_mfunc;                                      \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#define DECLARE_DELEGATE_1(name, R, A)                                      \
    typedef R (*name##_cb)(void*, A);                                       \
    struct name {                                                           \
        union {                                                             \
            R (__fastcall* mfunc)(void*,int,A);                             \
            R (*func)(void*,A);                                             \
        };                                                                  \
        void* obj;                                                          \
        unsigned int use_mfunc : 1;                                         \
    };                                                                      \
    template <class F, class O>                                             \
    static name Bind_##name(F func, O obj) {                                \
        Delegate1<R, A> cpp = Bind(func, obj);                              \
        struct name ret;                                                    \
        ret.func = cpp.func;                                                \
        ret.obj = cpp.obj;                                                  \
        ret.use_mfunc = cpp.use_mfunc;                                      \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#define DECLARE_DELEGATE_2(name, R, A, B)                                   \
    typedef R (*name##_cb)(void*, A, B);                                    \
    struct name {                                                           \
        union {                                                             \
            R (__fastcall* mfunc)(void*,int,A,B);                           \
            R (*func)(void*,A,B);                                           \
        };                                                                  \
        void* obj;                                                          \
        unsigned int use_mfunc : 1;                                         \
    };                                                                      \
    template <class F, class O>                                             \
    static name Bind_##name(F func, O obj) {                                \
        Delegate2<R, A, B> cpp = Bind(func, obj);                           \
        struct name ret;                                                    \
        ret.func = cpp.func;                                                \
        ret.obj = cpp.obj;                                                  \
        ret.use_mfunc = cpp.use_mfunc;                                      \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

/* ------------------------------------------------------------------------- */
#elif !defined  __cplusplus && defined DELEGATE_HAS_MFUNC

#define DECLARE_DELEGATE_0(name, R)                                         \
    typedef R (*name##_cb)(void*);                                          \
    struct name {                                                           \
        union {                                                             \
            R (__fastcall* mfunc)(void*,int);                               \
            R (*func)(void*);                                               \
        };                                                                  \
        void* obj;                                                          \
        unsigned int use_mfunc : 1;                                         \
    };                                                                      \
    static struct name Bind_##name(name##_cb func, void* obj) {             \
        struct name ret;                                                    \
        ret.func = func;                                                    \
        ret.obj = obj;                                                      \
        ret.use_mfunc = 0;                                                  \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#define DECLARE_DELEGATE_1(name, R, A)                                      \
    typedef R (*name##_cb)(void*, A);                                       \
    struct name {                                                           \
        union {                                                             \
            R (__fastcall* mfunc)(void*,int,A);                             \
            R (*func)(void*,A);                                             \
        };                                                                  \
        void* obj;                                                          \
        unsigned int use_mfunc : 1;                                         \
    };                                                                      \
    static struct name Bind_##name(name##_cb func, void* obj) {             \
        struct name ret;                                                    \
        ret.func = func;                                                    \
        ret.obj = obj;                                                      \
        ret.use_mfunc = 0;                                                  \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#define DECLARE_DELEGATE_2(name, R, A, B)                                   \
    typedef R (*name##_cb)(void*, A, B);                                    \
    struct name {                                                           \
        union {                                                             \
            R (__fastcall* mfunc)(void*,int,A,B);                           \
            R (*func)(void*,A,B);                                           \
        };                                                                  \
        void* obj;                                                          \
        unsigned int use_mfunc : 1;                                         \
    };                                                                      \
    static struct name Bind_##name(name##_cb func, void* obj) {             \
        struct name ret;                                                    \
        ret.func = func;                                                    \
        ret.obj = obj;                                                      \
        ret.use_mfunc = 0;                                                  \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

/* ------------------------------------------------------------------------- */
#elif defined __cplusplus && !defined DELEGATE_HAS_MFUNC

#define DECLARE_DELEGATE_0(name, R)                                         \
    typedef R (*name##_cb)(void*);                                          \
    struct name {                                                           \
        R (*func)(void*);                                                   \
        void* obj;                                                          \
    };                                                                      \
    template <class F, class O>                                             \
    static name Bind_##name(F func, O obj) {                                \
        Delegate0<R> cpp = Bind(func, obj);                                 \
        struct name ret;                                                    \
        ret.func = cpp.func;                                                \
        ret.obj = cpp.obj;                                                  \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#define DECLARE_DELEGATE_1(name, R, A)                                      \
    typedef R (*name##_cb)(void*, A);                                       \
    struct name {                                                           \
        R (*func)(void*,A);                                                 \
        void* obj;                                                          \
    };                                                                      \
    template <class F, class O>                                             \
    static name Bind_##name(F func, O obj) {                                \
        Delegate1<R, A> cpp = Bind(func, obj);                              \
        struct name ret;                                                    \
        ret.func = cpp.func;                                                \
        ret.obj = cpp.obj;                                                  \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#define DECLARE_DELEGATE_2(name, R, A, B)                                   \
    typedef R (*name##_cb)(void*, A, B);                                    \
    struct name {                                                           \
        R (*func)(void*,A,B);                                               \
        void* obj;                                                          \
    };                                                                      \
    template <class F, class O>                                             \
    static name Bind_##name(F func, O obj) {                                \
        Delegate2<R, A, B> cpp = Bind(func, obj);                           \
        struct name ret;                                                    \
        ret.func = cpp.func;                                                \
        ret.obj = cpp.obj;                                                  \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

/* ------------------------------------------------------------------------- */
#elif !defined __cplusplus && !defined DELEGATE_HAS_MFUNC

#define DECLARE_DELEGATE_0(name, R)                                         \
    typedef R (*name##_cb)(void*);                                          \
    struct name {                                                           \
        R (*func)(void*);                                                   \
        void* obj;                                                          \
    };                                                                      \
    static struct name Bind_##name(name##_cb func, void* obj) {             \
        struct name ret;                                                    \
        ret.func = func;                                                    \
        ret.obj = obj;                                                      \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#define DECLARE_DELEGATE_1(name, R, A)                                      \
    typedef R (*name##_cb)(void*, A);                                       \
    struct name {                                                           \
        R (*func)(void*,A);                                                 \
        void* obj;                                                          \
    };                                                                      \
    static struct name Bind_##name(name##_cb func, void* obj) {             \
        struct name ret;                                                    \
        ret.func = func;                                                    \
        ret.obj = obj;                                                      \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#define DECLARE_DELEGATE_2(name, R, A, B)                                   \
    typedef R (*name##_cb)(void*, A, B);                                    \
    struct name {                                                           \
        R (*func)(void*,A,B);                                               \
        void* obj;                                                          \
    };                                                                      \
    static struct name Bind_##name(name##_cb func, void* obj) {             \
        struct name ret;                                                    \
        ret.func = func;                                                    \
        ret.obj = obj;                                                      \
        return ret;                                                         \
    }                                                                       \
    typedef struct name name

#endif

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
#define BIND0(name, func, obj) Bind_##name(func, obj)
#define BIND1(name, func, obj, PA) Bind_##name(func, obj)
#define BIND2(name, func, obj, PA, PB) Bind_##name(func, obj)

#else
#define BIND0(name, func, obj)                                              \
    (                                                                       \
     (void) (0 && ((func)(obj), 0)),                                        \
     Bind_##name((name##_cb) (func), obj)                                   \
    )

#define BIND1(name, func, obj, PA)                                          \
    (                                                                       \
     (void) (0 && ((func)(obj, *(PA) NULL), 0)),                            \
     Bind_##name((name##_cb) (func), obj)                                   \
    )

#define BIND2(name, func, obj, PA, PB)                                      \
    (                                                                       \
     (void) (0 && ((func)(obj, *(PA) NULL, *(PB) NULL), 0)),                \
     Bind_##name((name##_cb) (func), obj)                                   \
    )

#endif

/* ------------------------------------------------------------------------- */

DECLARE_DELEGATE_0(VoidDelegate, void);
DECLARE_DELEGATE_0(IntDelegate, int);

#define BindVoid(func, obj) BIND0(VoidDelegate, func, obj)
#define BindInt(func, obj) BIND0(IntDelegate, func, obj)

