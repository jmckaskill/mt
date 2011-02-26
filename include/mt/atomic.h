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

/* Disable fals warnings generated by msvc for their interlocked commands:
 * 64 bit portability issues
 */
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable:4311)
#   pragma warning(disable:4312)
#endif

#if defined _MSC_VER
#include <intrin.h>

#ifdef _WIN64
/* Returns previous value */
#   define MT_AtomicSetPtr(pval, new_val) \
        (void*) _InterlockedExchange64((__int64*) (pval), (__int64) (new_val))

/* Returns previous value */
#   define MT_AtomicSetPtrFrom(pval, from, to) \
        (void*) _InterlockedCompareExchange64((__int64*) (pval), (__int64) (from), (__int64) (to))
#else
/* Returns previous value */
#   define MT_AtomicSetPtr(pval, new_val) \
        (void*) MT_AtomicSet((MT_AtomicInt*) (pval), (long) (new_val))

/* Returns previous value */
#   define MT_AtomicSetPtrFrom(pval, from, to) \
        (void*) MT_AtomicSetFrom((MT_AtomicInt*) (pval), (long) (from), (long) (to))
#endif

/* Returns previous value */
MT_INLINE long MT_AtomicSet(MT_AtomicInt* a, long val)
{
    return _InterlockedExchange(a, val);
}

/* Returns previous value */
MT_INLINE long MT_AtomicSetFrom(MT_AtomicInt* a, long from, long to)
{
    return _InterlockedCompareExchange(a, to, from);
}

/* Returns the new value */
MT_INLINE long MT_AtomicIncrement(MT_AtomicInt* a)
{
    return _InterlockedIncrement((long*) a);
}

/* Returns the new value */
MT_INLINE long MT_AtomicDecrement(MT_AtomicInt* a)
{
    return _InterlockedDecrement((long*) a);
}

/* Returns the new value */
MT_INLINE long MT_AtomicAdd(MT_AtomicInt* a, long incr)
{
    long new_value;

    do {
        new_value = *a + incr;
    } while (MT_AtomicSetFrom(a, *a, new_value) == new_value);

    return new_value;
}

/* Returns the new value */
MT_INLINE long MT_AtomicSubtract(MT_AtomicInt* a, long decr)
{
    return MT_AtomicAdd(a, -decr);
}

#elif defined __GNUC__

/* Returns previous value */
#ifdef __llvm__
#   define MT_AtomicSetPtr(pval, new_val) ((void*) __sync_lock_test_and_set(pval, new_val))
#else
#   define MT_AtomicSetPtr(pval, new_val) __sync_lock_test_and_set(pval, new_val)
#endif

/* Returns previous value */
#ifdef __llvm__
#   define MT_AtomicSetPtrFrom(pval, from, to) ((void*) __sync_val_compare_and_swap(pval, from, to))
#else
#   define MT_AtomicSetPtrFrom(pval, from, to)  __sync_val_compare_and_swap(pval, from, to)
#endif

/* Returns previous value */
MT_INLINE long MT_AtomicSet(MT_AtomicInt* a, long val)
{
    return __sync_lock_test_and_set(a, val);
}

/* Returns previous value */
MT_INLINE long MT_AtomicSetFrom(MT_AtomicInt* a, long from, long to)
{
    return __sync_val_compare_and_swap(a, from, to);
}

/* Returns the new value */
MT_INLINE long MT_AtomicIncrement(MT_AtomicInt* a)
{
    return __sync_add_and_fetch(a, 1);
}

/* Returns the new value */
MT_INLINE long MT_AtomicDecrement(MT_AtomicInt* a)
{
    return __sync_sub_and_fetch(a, 1);
}

/* Returns the new value */
MT_INLINE long MT_AtomicAdd(MT_AtomicInt* a, long incr)
{
    return __sync_add_and_fetch(a, incr);
}

/* Returns the new value */
MT_INLINE long MT_AtomicSubtract(MT_AtomicInt* a, long decr)
{
    return __sync_sub_and_fetch(a, decr);
}

#else
#error
#endif


