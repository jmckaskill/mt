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

#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

/* ------------------------------------------------------------------------- */

/* Declares a new vector d_Vector(name) and slice d_Slice(name) that holds
 * 'type' data values
 */
#ifdef __cplusplus 

#define DVECTOR_INIT(name, type)                                            \
                                                                            \
    struct d_slice_##name {                                                 \
        int size;                                                           \
        type const* data;                                                   \
                                                                            \
        bool operator == (d_slice_##name r) const {                         \
            return size == r.size && 0 == memcmp(data, r.data, size * sizeof(type));\
        }                                                                   \
    };                                                                      \
                                                                            \
    struct d_vector_##name {                                                \
        int size;                                                           \
        type* data;                                                         \
                                                                            \
        operator d_slice_##name () const {                                  \
            d_slice_##name ret;                                             \
            ret.data = data;                                                \
            ret.size = size;                                                \
            return ret;                                                     \
        }                                                                   \
    }

#else

#define DVECTOR_INIT(name, type)                                            \
    typedef struct d_vector_##name d_vector_##name;                         \
    typedef struct d_vector_##name d_slice_##name;                          \
                                                                            \
    struct d_vector_##name {                                                \
        int size;                                                           \
        type* data;                                                         \
    }

#endif

/* Static initializer for vectors 'd_Vector(name) foo = DV_INIT;' */
#define DV_INIT {0,NULL}
#define d_Vector(name) d_vector_##name
#define d_Slice(name) d_slice_##name

DVECTOR_INIT(int, int);


/* ------------------------------------------------------------------------- */

#define dv_datasize(VEC) ((int) sizeof((VEC).data[0]))
#define dv_pdatasize(PVEC) ((int) sizeof((PVEC)->data[0]))
#define dv_reserve(PVEC, NEWSZ) ((PVEC)->data = dv_resize_base((PVEC)->data, (NEWSZ) * dv_pdatasize(PVEC)))

/* ------------------------------------------------------------------------- */

/* Frees the data in 'VEC'*/
#define dv_free(VEC) dv_free_base((VEC).data)

/* ------------------------------------------------------------------------- */

/* Resets the vector 'PVEC' to size zero */
#define dv_clear(PVEC) ((void) ((PVEC)->size = 0, (PVEC)->data && (*((uint64_t*) (PVEC)->data) = 0)))

/* ------------------------------------------------------------------------- */

/* Resizes the vector to hold 'newsz' values */
#define dv_resize(PVEC, NEWSZ) ((PVEC)->size = NEWSZ, dv_reserve(PVEC, (PVEC)->size))

/* ------------------------------------------------------------------------- */

/* Adds space for NUM values at 'INDEX' in the vector and returns a pointer to
 * the added space - INDEX and NUM are evaluated only once before the resize
 * occurs. The _zeroed form also zeros the added buffer.
 */
#define dv_insert_buffer(PVEC, INDEX, NUM) dv_insert_buffer_(&(PVEC)->data, &(PVEC)->size, INDEX, NUM, dv_pdatasize(PVEC))
#define dv_insert_zeroed(PVEC, INDEX, NUM) dv_insert_zeroed_(&(PVEC)->data, &(PVEC)->size, INDEX, NUM, dv_pdatasize(PVEC))

/* Adds space for NUM values at the end of the vector and returns a pointer to
 * the beginning of the added space - NUM is only evaluated once before the
 * append occurs. The _zeroed form also zeros the added buffer.
 */
#define dv_append_buffer(PVEC, NUM) dv_append_buffer_(&(PVEC)->data, &(PVEC)->size, NUM, dv_pdatasize(PVEC))
#define dv_append_zeroed(PVEC, NUM) dv_append_zeroed_(&(PVEC)->data, &(PVEC)->size, NUM, dv_pdatasize(PVEC))

/* ------------------------------------------------------------------------- */

/* Appends the data 'DATA' of size 'SZ' to the vector 'PTO'. 'DATA' and 'PTO'
 * must be the same type - DATA and SZ are only evaluated once.
 */
#define dv_append2(PTO, DATA, SZ)                                           \
    do {                                                                    \
        STATIC_ASSERT(sizeof((DATA)[0]) == dv_pdatasize(PTO));              \
        const void* _data_ = DATA;                                          \
        int _sz_ = SZ;                                                      \
        (PTO)->data = dv_resize_base((PTO)->data, dv_pdatasize(PTO) * (_sz_ + (PTO)->size)); \
        memcpy((PTO)->data + (PTO)->size, _data_, _sz_ * dv_pdatasize(PTO));\
        (PTO)->size += _sz_;                                                \
    } while(0)

/* ------------------------------------------------------------------------- */

#define dv_append1(PTO, VALUE)                                              \
    do {                                                                    \
        (PTO)->data = dv_resize_base((PTO)->data, dv_pdatasize(PTO) * ((PTO)->size + 1));\
        (PTO)->data[(PTO)->size] = (VALUE);                                 \
        (PTO)->size += 1;                                                   \
    } while (0)

/* Appends the vector FROM to PTO - they must be of the same type - FROM is
 * only evaluated once.
 */

#ifdef __cplusplus
template <class To, class From>
void dv_append(To* PTO, From _from)
{ dv_append2(PTO, _from.data, _from.size); }

#else
#define dv_append(PTO, FROM)                                                \
    do {                                                                    \
        void* _todata, *_fromdata;                                          \
        int _tosz, _fromsz;                                                 \
        _todata = (PTO)->data;                                              \
        _tosz = (PTO)->size;                                                \
        *(PTO) = (FROM);                                                    \
        _fromdata = (PTO)->data;                                            \
        _fromsz = (PTO)->size;                                              \
        (PTO)->data = dv_resize_base(_todata, dv_pdatasize(PTO) * (_tosz + _fromsz)); \
        memcpy((PTO)->data + _tosz, _fromdata, _fromsz * dv_pdatasize(PTO));\
        (PTO)->size = _fromsz + _tosz;                                      \
    } while (0)
#endif

/* ------------------------------------------------------------------------- */

/* Sets 'PTO' to be a copy of 'DATA' of size 'SZ' - DATA and SZ are only
 * evaluated once.
 */
#define dv_set2(PTO, DATA, SZ) do { (PTO)->size = 0; dv_append2(PTO, DATA, SZ); } while(0)

/* ------------------------------------------------------------------------- */

/* Sets 'PTO' to be a copy of 'FROM' */
#define dv_set(PTO, FROM) do { (PTO)->size = 0; dv_append(PTO, FROM); } while(0)

/* ------------------------------------------------------------------------- */

/* Inserts a copy of 'DATA' of size 'SZ' at index 'INDEX' in 'PTO' */
#define dv_insert2(PTO, INDEX, DATA, SZ)                                    \
    do {                                                                    \
        STATIC_ASSERT(sizeof((DATA)[0]) == dv_pdatasize(PTO));              \
        const void* _fromdata = (DATA);                                     \
        int _fromsz = (SZ);                                                 \
        void* _todata = dv_insert_buffer(PTO, INDEX, _fromsz);              \
        memcpy(_todata, _fromdata, _fromsz * dv_pdatasize(PTO));            \
    } while(0)

/* ------------------------------------------------------------------------- */

/* Inserts a copy of 'FROM' at index 'INDEX' in 'PTO' */

#ifdef __cplusplus
template <class To, class From>
void dv_insert(To* PTO, int INDEX, From FROM)
{ dv_insert2(PTO, INDEX, FROM.data, FROM.size); }

#else
#define dv_insert(PTO, INDEX, FROM)                                         \
    do {                                                                    \
        void* buf;                                                          \
        void* _todata, *_fromdata;                                          \
        int _tosz, _fromsz;                                                 \
        _todata = (PTO)->data;                                              \
        _tosz = (PTO)->size;                                                \
        *(PTO) = (FROM);                                                    \
        _fromdata = (PTO)->data;                                            \
        _fromsz = (PTO)->size;                                              \
        (PTO)->data = _todata;                                              \
        (PTO)->size = _tosz;                                                \
        buf = dv_insert_buffer(PTO, INDEX, _fromsz);                        \
        memcpy(buf, _fromdata, _fromsz * dv_pdatasize(PTO));                \
    } while (0)
#endif

/* ------------------------------------------------------------------------- */

/* Erases 'NUM' values beginning with data[INDEX] */
#define dv_erase(PVEC, INDEX, NUM)                                          \
    do {                                                                    \
        int _from = (int) (INDEX);                                          \
        int _num = (int) (NUM);                                             \
        int _to = _from + _num;                                             \
        memmove(&(PVEC)->data[_from], &(PVEC)->data[_to], ((PVEC)->size - _to) * dv_pdatasize(PVEC)); \
        dv_resize(PVEC, (PVEC)->size - _num);                               \
    } while(0)

/* Erases 'NUM' values FROM the end of the vector */
#define dv_erase_end(PVEC, NUM)  dv_resize(PVEC, (PVEC)->size - (NUM))

/* ------------------------------------------------------------------------- */

/* Removes all values that equal 'val' in the vector 'pvec' */
#define dv_remove(pvec, val) dv_remove2(pvec, (pvec)->data[INDEX] == (val))

/* Removes all items for which 'test' resolves to true - use INDEX to
 * reference the current index. eg test could be
 * "should_remove(&myvec.data[INDEX])"
 */
#define dv_remove2(pvec, test)                                              \
    do {                                                                    \
        int INDEX;                                                          \
        for (INDEX = 0; INDEX < (pvec)->size;) {                            \
            if (test) {                                                     \
                dv_erase(pvec, INDEX, 1);                                   \
            } else {                                                        \
                INDEX++;                                                    \
            }                                                               \
        }                                                                   \
    } while (0)                                                             \
 
/* ------------------------------------------------------------------------- */

/* Sets 'pidx' to the index of the value which equals 'val' in 'vec' or -1 if
 * none is found. 'pidx' should point to an int.
 */
#define dv_find(vec, val, pidx) dv_find2(vec, (vec).data[INDEX] == (val), pidx)

/* Iterates over the vector values until 'test' evaluates to true. 'test'
 * should use the local variable INDEX to reference the index. eg test could
 * be "test_item(&myvec.data[INDEX])". 'pidx' is then set to the found index
 * or -1 if none is found. 'pidx' should point to an int.
 */
#define dv_find2(vec, test, pidx)                                           \
    do {                                                                    \
        int INDEX;                                                          \
        *(pidx) = -1;                                                       \
        for (INDEX = 0; INDEX < (vec).size; INDEX++) {                      \
            if (test) {                                                     \
                *(pidx) = INDEX;                                            \
                break;                                                      \
            }                                                               \
        }                                                                   \
    } while (0)                                                             \
 


/* ------------------------------------------------------------------------- */

/* Does a memcmp between the data in VEC1 and VEC2 */
#define dv_cmp(VEC1, VEC2) ((VEC1).size == (VEC2).size ? memcmp((VEC1).data, (VEC2).data, (VEC1).size * dv_datasize(VEC1)) : (VEC2).size - (VEC1).size)

/* Returns VEC1 == VEC2 */
#define dv_equals(VEC1, VEC2) ((VEC1).size == (VEC2).size && 0 == memcmp((VEC1).data, (VEC2).data, (VEC1).size * dv_datasize(VEC1)))

/* Returns the last value in the vector - note the vector must be non-empty. */
#define dv_last(VEC) ((VEC).data[(VEC).size - 1])













/* Implementation declarations and inlines */

/* ------------------------------------------------------------------------- */

DMEM_API void* dv_resize_base(void* p, int newsz);
DMEM_API void dv_free_base(void* p);

#ifdef __cplusplus
template <class T>
T* dv_resize_base(T* p, int newsz)
{ return (T*) dv_resize_base((void*) p, newsz); }
#endif

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
template <class T>
DMEM_INLINE T* dv_insert_buffer_(T** data, int* size, int off, int num, int datasize)
{
    *data = (T*) dv_resize_base(*data, (*size + num) * datasize);
    memmove(*data + off + num, *data + off, (*size - off) * datasize);
    *size += num;
    return *data + off;
}

template <class T>
DMEM_INLINE T* dv_insert_zeroed_(T** data, int* size, int off, int num, int datasize)
{
    *data = (T*) dv_resize_base(data, (*size + num) * datasize);
    memmove(*data + off + num, *data + off, (*size - off) * datasize);
    memset(*data + off, 0, num * datasize);
    *size += num;
    return *data + off;
}

template <class T>
DMEM_INLINE T* dv_append_buffer_(T** data, int* size, int num, int datasize)
{
    int sz = *size;
    *data = (T*) dv_resize_base(data, (*size + num) * datasize);
    *size += num;
    return *data + sz;
}

template <class T>
DMEM_INLINE T* dv_append_zeroed_(T** data, int* size, int num, int datasize)
{
    int sz = *size;
    *data = (T*) dv_resize_base(data, (*size + num) * datasize);
    memset(*data + sz, 0, num * datasize);
    *size += num;
    return *data + sz;
}

/* ------------------------------------------------------------------------- */

#else /* !__cplusplus */
DMEM_INLINE void* dv_insert_buffer_(void* pdata, int* size, int off, int num, int datasize)
{
    char** data = (char**) pdata;
    *data = dv_resize_base(*data, (*size + num) * datasize);
    memmove(*data + (datasize * (off + num)), *data + (datasize * off), (*size - off) * datasize);
    *size += num;
    return *data + (off * datasize);
}

DMEM_INLINE void* dv_insert_zeroed_(void* pdata, int* size, int off, int num, int datasize)
{
    char** data = (char**) pdata;
    *data = dv_resize_base(*data, (*size + num) * datasize);
    memmove(*data + (datasize * (off + num)), *data + (datasize * off), (*size - off) * datasize);
    memset(*data + (off * datasize), 0, num * datasize);
    *size += num;
    return *data + (off * datasize);
}

DMEM_INLINE void* dv_append_buffer_(void* pdata, int* size, int num, int datasize)
{
    char** data = (char**) pdata;
    int sz = *size;
    *data = dv_resize_base(*data, (*size + num) * datasize);
    *size += num;
    return *data + (sz * datasize);
}

DMEM_INLINE void* dv_append_zeroed_(void* pdata, int* size, int num, int datasize)
{
    char** data = (char**) pdata;
    int sz = *size;
    *data = dv_resize_base(*data, (*size + num) * datasize);
    memset(*data + (sz * datasize), 0, num * datasize);
    *size += num;
    return *data + (sz * datasize);
}

#endif


