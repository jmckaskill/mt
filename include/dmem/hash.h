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

#include "common.h"
#include "char.h"
#include <stdint.h>

/* ------------------------------------------------------------------------- */

typedef uint32_t khint_t;
typedef struct dh_base dh_base;

struct dh_base {
    khint_t n_buckets, size, n_occupied, upper_bound;
    uint32_t *flags;
    size_t idx;
};

/* ------------------------------------------------------------------------- */

DMEM_API void dh_erase_base(dh_base* h, size_t idx);
DMEM_API void dh_clear_base(dh_base* h);
DMEM_API void dh_free_base(dh_base* h, void* keys, void* vals);
DMEM_API bool dh_hasnext_base(const dh_base* h, int* pidx);

/* ------------------------------------------------------------------------- */

DMEM_API bool dhi_get_base(const dh_base* h, intmax_t key);
DMEM_API bool dhi_add_base(dh_base* h, intmax_t key, size_t valsz);

/* ------------------------------------------------------------------------- */

DMEM_API bool dhs_get_base(const dh_base* h, d_Slice(char) key);
DMEM_API bool dhs_add_base(dh_base* h, d_Slice(char) key, size_t valsz);

/* ------------------------------------------------------------------------- */

#define DHASH_INIT_INT(name, khval_t)                                         \
    typedef struct {                                                          \
        dh_base base;                                                         \
        intmax_t *keys;                                                       \
        khval_t* vals;                                                        \
    } dhi_##name##_t

#define DHASH_INIT_STR(name, khval_t)                                         \
    typedef struct {                                                          \
        dh_base base;                                                         \
        d_Slice(char) *keys;                                                  \
        khval_t* vals;                                                        \
    } dhs_##name##_t

#define d_IntHash(name) dhi_##name##_t
#define d_StringHash(name) dhs_##name##_t

/* ------------------------------------------------------------------------- */

#define dh_size(h)              ((h)->base.size)
#define dh_clear(h)             dh_clear_base(&(h)->base)
#define dh_free(h)              dh_free_base(&(h)->base, (h)->keys, (h)->vals)
#define dh_hasnext(h, pidx)     dh_hasnext_base(&(h)->base, pidx)
#define dh_erase(h, idx)        dh_erase_base(&(h)->base, idx)

#define dhi_get(h, key, pval)   (dhi_get_base(&(h)->base, key) && (*(pval) = (h)->vals[(h)->base.idx], true))
#define dhi_find(h, key, pidx)  (dhi_get_base(&(h)->base, key) && (*(pidx) = (h)->base.idx, true))
#define dhi_remove(h, key)      (dhi_get_base(&(h)->base, key) && (dh_erase_base(&(h)->base, (h)->base.idx), true))
#define dhi_add(h, key, pidx)   (dhi_add_base(&(h)->base, key, sizeof((h)->vals[0])) ? ((*(pidx) = (h)->base.idx), true) : ((*(pidx) = (h)->base.idx), false))
#define dhi_set(h, key, val)    (dhi_add_base(&(h)->base, key, sizeof((h)->vals[0])), ((h)->vals[(h)->base.idx] = (val)))

#define dhs_get(h, key, pval)   (dhs_get_base(&(h)->base, key) && (*(pval) = (h)->vals[(h)->base.idx], true))
#define dhs_find(h, key, pidx)  (dhs_get_base(&(h)->base, key) && (*(pidx) = (h)->base.idx, true))
#define dhs_remove(h, key)      (dhs_get_base(&(h)->base, key) && (dh_erase_base(&(h)->base, (h)->base.idx), true))
#define dhs_add(h, key, pidx)   (dhs_add_base(&(h)->base, key, sizeof((h)->vals[0])) ? ((*(pidx) = (h)->base.idx), true) : ((*(pidx) = (h)->base.idx), false))
#define dhs_set(h, key, val)    (dhs_add_base(&(h)->base, key, sizeof((h)->vals[0])), ((h)->vals[(h)->base.idx] = (val)))

