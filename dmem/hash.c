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

#define DMEM_LIBRARY
#include <dmem/hash.h>

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <alloca.h>
#endif

typedef khint_t dh_Iter;

#define __ac_HASH_PRIME_SIZE 32
static const uint32_t __ac_prime_list[__ac_HASH_PRIME_SIZE] = {
    0ul,          3ul,          11ul,         23ul,         53ul,
    97ul,         193ul,        389ul,        769ul,        1543ul,
    3079ul,       6151ul,       12289ul,      24593ul,      49157ul,
    98317ul,      196613ul,     393241ul,     786433ul,     1572869ul,
    3145739ul,    6291469ul,    12582917ul,   25165843ul,   50331653ul,
    100663319ul,  201326611ul,  402653189ul,  805306457ul,  1610612741ul,
    3221225473ul, 4294967291ul
};

#define __ac_isempty(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&2)
#define __ac_isdel(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&1)
#define __ac_iseither(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&3)
#define __ac_set_isdel_false(flag, i) (flag[i>>4]&=~(1ul<<((i&0xfU)<<1)))
#define __ac_set_isempty_false(flag, i) (flag[i>>4]&=~(2ul<<((i&0xfU)<<1)))
#define __ac_set_isboth_false(flag, i) (flag[i>>4]&=~(3ul<<((i&0xfU)<<1)))
#define __ac_set_isdel_true(flag, i) (flag[i>>4]|=1ul<<((i&0xfU)<<1))

#ifndef NDEBUG
#define __ac_reset_field_debug(vec, i) memset(&vec[i], 0xCD, sizeof(vec[i]))
#else
#define __ac_reset_field_debug(vec, i)
#endif

static const double __ac_HASH_UPPER = 0.77;

#define val_ptr(vals, valsz, i) ((char*) (vals) + (i * valsz))

typedef struct dhi_impl_t dhi_impl_t;
typedef struct dhs_impl_t dhs_impl_t;

struct dhi_impl_t {
    dh_base base;
    intmax_t* keys;
    void* vals;
};

struct dhs_impl_t {
    dh_base base;
    d_Slice(char)* keys;
    void* vals;
};

/* ------------------------------------------------------------------------- */

#if INTMAX_MAX == INT32_MAX + 0
static uint32_t hash_int(intmax_t key)
{ 
  return (uint32_t) key; 
}
#elif INTMAX_MAX == INT64_MAX + 0
static uint32_t hash_int(intmax_t key)
{
  return (uint32_t) ((key >> 33) ^ key ^ (key << 11));
}
#else
#error
#endif

/* ------------------------------------------------------------------------- */

static uint32_t hash_string(d_Slice(char) key)
{

    int i;
    uint32_t h;

    if (key.size == 0) {
        return 0;
    }

    h = *key.data;

    for (i = 1; i < key.size; i++) {
        h = (h << 5) - h + key.data[i];
    }

    return h;
}

/* ------------------------------------------------------------------------- */

void dh_free_base(dh_base* h, void* keys, void* vals)
{
    if (h) {
        free(h->flags);
        free(keys);
        free(vals);
    }
}

/* ------------------------------------------------------------------------- */

void dh_clear_base(dh_base* h)
{
    if (h && h->flags) {
        memset(h->flags, 0xaa, ((h->n_buckets>>4) + 1) * sizeof(uint32_t));
        h->size = h->n_occupied = 0;
    }
}

/* ------------------------------------------------------------------------- */

bool dhi_get_base(const dh_base* h, intmax_t key)
{
    dhi_impl_t* hi = (dhi_impl_t*) h;
    if (h->n_buckets) {
        khint_t inc, k, i, last;
        k = hash_int(key); i = k % h->n_buckets;
        inc = 1 + k % (h->n_buckets - 1); last = i;
        while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || hi->keys[i] != key)) {
            if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets;
            else i += inc;
            if (i == last) return false;
        }

        if (__ac_iseither(h->flags, i)) {
            return false;
        }

        hi->base.idx = i;
        return true;
    } else {
        return false;
    }
}


bool dhs_get_base(const dh_base* h, d_Slice(char) key)
{
    dhs_impl_t* hi = (dhs_impl_t*) h;
    if (h->n_buckets) {
        khint_t inc, k, i, last;
        k = hash_string(key); i = k % h->n_buckets;
        inc = 1 + k % (h->n_buckets - 1); last = i;
        while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !dv_equals(hi->keys[i], key))) {
            if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets;
            else i += inc;
            if (i == last) return false;
        }

        if (__ac_iseither(h->flags, i)) {
            return false;
        }

        hi->base.idx = i;
        return true;
    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------- */

static void resize_int(dh_base *h, khint_t new_n_buckets, size_t valsz)
{
    dhi_impl_t* hi = (dhi_impl_t*) h;
    uint32_t *new_flags = 0;
    khint_t j = 1;
    {
        khint_t t = __ac_HASH_PRIME_SIZE - 1;
        while (__ac_prime_list[t] > new_n_buckets) --t;
        new_n_buckets = __ac_prime_list[t+1];
        if (h->size >= (khint_t)(new_n_buckets * __ac_HASH_UPPER + 0.5)) j = 0;
        else {
            new_flags = (uint32_t*)malloc(((new_n_buckets>>4) + 1) * sizeof(uint32_t));
            memset(new_flags, 0xaa, ((new_n_buckets>>4) + 1) * sizeof(uint32_t));
            if (h->n_buckets < new_n_buckets) {
                hi->keys = (intmax_t*)realloc(hi->keys, new_n_buckets * sizeof(intmax_t));
                hi->vals = realloc(hi->vals, new_n_buckets * valsz);
            }
        }
    }
    if (j) {
        char* val = (char*) alloca(valsz);
        char* valtmp = (char*) alloca(valsz);
        for (j = 0; j != h->n_buckets; ++j) {
            if (__ac_iseither(h->flags, j) == 0) {
                intmax_t key = ((intmax_t*) hi->keys)[j];
                memcpy(val, val_ptr(hi->vals, valsz, j), valsz);
                __ac_set_isdel_true(h->flags, j);
                while (1) {
                    khint_t inc, k, i;
                    k = hash_int(key);
                    i = k % new_n_buckets;
                    inc = 1 + k % (new_n_buckets - 1);
                    while (!__ac_isempty(new_flags, i)) {
                        if (i + inc >= new_n_buckets) i = i + inc - new_n_buckets;
                        else i += inc;
                    }
                    __ac_set_isempty_false(new_flags, i);
                    if (i < h->n_buckets && __ac_iseither(h->flags, i) == 0) {
                        intmax_t tmp = hi->keys[i];
                        hi->keys[i] = key;
                        key = tmp;
                        {
                            char* tmp;
                            memcpy(valtmp, val_ptr(hi->vals, valsz, i), valsz);
                            memcpy(val_ptr(hi->vals, valsz, i), val, valsz);
                            tmp = valtmp;
                            valtmp = val;
                            val = tmp;
                        }
                        __ac_set_isdel_true(h->flags, i);
                    } else {
                        hi->keys[i] = key;
                        memcpy(val_ptr(hi->vals, valsz, i), val, valsz);
                        break;
                    }
                }
            }
        }
        if (h->n_buckets > new_n_buckets) {
            hi->keys = (intmax_t*)realloc(hi->keys, new_n_buckets * sizeof(intmax_t));
            hi->vals = realloc(hi->vals, new_n_buckets * valsz);
        }
        free(h->flags);
        h->flags = new_flags;
        h->n_buckets = new_n_buckets;
        h->n_occupied = h->size;
        h->upper_bound = (khint_t)(h->n_buckets * __ac_HASH_UPPER + 0.5);
    }
}

static void resize_string(dh_base *h, khint_t new_n_buckets, size_t valsz)
{
    dhs_impl_t* hi = (dhs_impl_t*) h;
    uint32_t *new_flags = 0;
    khint_t j = 1;
    {
        khint_t t = __ac_HASH_PRIME_SIZE - 1;
        while (__ac_prime_list[t] > new_n_buckets) --t;
        new_n_buckets = __ac_prime_list[t+1];
        if (h->size >= (khint_t)(new_n_buckets * __ac_HASH_UPPER + 0.5)) j = 0;
        else {
            new_flags = (uint32_t*)malloc(((new_n_buckets>>4) + 1) * sizeof(uint32_t));
            memset(new_flags, 0xaa, ((new_n_buckets>>4) + 1) * sizeof(uint32_t));
            if (h->n_buckets < new_n_buckets) {
                hi->keys = (d_Slice(char)*)realloc(hi->keys, new_n_buckets * sizeof(d_Slice(char)));
                hi->vals = realloc(hi->vals, new_n_buckets * valsz);
            }
        }
    }
    if (j) {
        char* val = (char*) alloca(valsz);
        char* valtmp = (char*) alloca(valsz);
        for (j = 0; j != h->n_buckets; ++j) {
            if (__ac_iseither(h->flags, j) == 0) {
                d_Slice(char) key = hi->keys[j];
                memcpy(val, val_ptr(hi->vals, valsz, j), valsz);
                __ac_set_isdel_true(h->flags, j);
                while (1) {
                    khint_t inc, k, i;
                    k = hash_string(key);
                    i = k % new_n_buckets;
                    inc = 1 + k % (new_n_buckets - 1);
                    while (!__ac_isempty(new_flags, i)) {
                        if (i + inc >= new_n_buckets) i = i + inc - new_n_buckets;
                        else i += inc;
                    }
                    __ac_set_isempty_false(new_flags, i);
                    if (i < h->n_buckets && __ac_iseither(h->flags, i) == 0) {
                        d_Slice(char) tmp = hi->keys[i];
                        hi->keys[i] = key;
                        key = tmp;
                        {
                            char* tmp;
                            memcpy(valtmp, val_ptr(hi->vals, valsz, i), valsz);
                            memcpy(val_ptr(hi->vals, valsz, i), val, valsz);
                            tmp = valtmp;
                            valtmp = val;
                            val = tmp;
                        }
                        __ac_set_isdel_true(h->flags, i);
                    } else {
                        hi->keys[i] = key;
                        memcpy(val_ptr(hi->vals, valsz, i), val, valsz);
                        break;
                    }
                }
            }
        }
        if (h->n_buckets > new_n_buckets) {
            hi->keys = (d_Slice(char)*)realloc(hi->keys, new_n_buckets * sizeof(d_Slice(char)));
            hi->vals = realloc(hi->vals, new_n_buckets * valsz);
        }
        free(h->flags);
        h->flags = new_flags;
        h->n_buckets = new_n_buckets;
        h->n_occupied = h->size;
        h->upper_bound = (khint_t)(h->n_buckets * __ac_HASH_UPPER + 0.5);
    }
}

/* ------------------------------------------------------------------------- */

bool dhi_add_base(dh_base* h, intmax_t key, size_t valsz)
{
    dhi_impl_t* hi = (dhi_impl_t*) h;
    khint_t x;
    if (h->n_occupied >= h->upper_bound) {
        if (h->n_buckets > (h->size<<1)) resize_int(h, h->n_buckets - 1, valsz);
        else resize_int(h, h->n_buckets + 1, valsz);
    }
    {
        khint_t inc, k, i, site, last;
        x = site = h->n_buckets; k = hash_int(key); i = k % h->n_buckets;
        if (__ac_isempty(h->flags, i)) x = i;
        else {
            inc = 1 + k % (h->n_buckets - 1); last = i;
            while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || hi->keys[i] != key)) {
                if (__ac_isdel(h->flags, i)) site = i;
                if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets;
                else i += inc;
                if (i == last) { x = site; break; }
            }
            if (x == h->n_buckets) {
                if (__ac_isempty(h->flags, i) && site != h->n_buckets) x = site;
                else x = i;
            }
        }
    }

    h->idx = x;

    if (__ac_isempty(h->flags, x)) {
        hi->keys[x] = key;
        __ac_set_isboth_false(h->flags, x);
        ++h->size; ++h->n_occupied;
        return true;

    } else if (__ac_isdel(h->flags, x)) {
        hi->keys[x] = key;
        __ac_set_isboth_false(h->flags, x);
        ++h->size;
        return true;

    } else {
        return false;
    }
}

bool dhs_add_base(dh_base* h, d_Slice(char) key, size_t valsz)
{
    dhs_impl_t* hi = (dhs_impl_t*) h;
    khint_t x;
    if (h->n_occupied >= h->upper_bound) {
        if (h->n_buckets > (h->size<<1)) resize_string(h, h->n_buckets - 1, valsz);
        else resize_string(h, h->n_buckets + 1, valsz);
    }
    {
        khint_t inc, k, i, site, last;
        x = site = h->n_buckets; k = hash_string(key); i = k % h->n_buckets;
        if (__ac_isempty(h->flags, i)) x = i;
        else {
            inc = 1 + k % (h->n_buckets - 1); last = i;
            while (!__ac_isempty(h->flags, i) && (__ac_isdel(h->flags, i) || !dv_equals(hi->keys[i], key))) {
                if (__ac_isdel(h->flags, i)) site = i;
                if (i + inc >= h->n_buckets) i = i + inc - h->n_buckets;
                else i += inc;
                if (i == last) { x = site; break; }
            }
            if (x == h->n_buckets) {
                if (__ac_isempty(h->flags, i) && site != h->n_buckets) x = site;
                else x = i;
            }
        }
    }

    h->idx = x;

    if (__ac_isempty(h->flags, x)) {
        hi->keys[x] = key;
        __ac_set_isboth_false(h->flags, x);
        ++h->size; ++h->n_occupied;
        return true;

    } else if (__ac_isdel(h->flags, x)) {
        hi->keys[x] = key;
        __ac_set_isboth_false(h->flags, x);
        ++h->size;
        return true;

    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------- */

void dh_erase_base(dh_base* h, size_t x)
{
    if (!__ac_iseither(h->flags, x)) {
        __ac_set_isdel_true(h->flags, x);
        --h->size;
    }
}

/* ------------------------------------------------------------------------- */

bool dh_hasnext_base(const dh_base* h, int* pidx)
{
    while (++(*pidx) < (int) h->n_buckets) {
        if (!__ac_iseither(h->flags, *pidx)) {
            return true;
        }
    }

    return false;
}
