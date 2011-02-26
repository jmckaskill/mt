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

#include "vector.hpp"
#include "hash.h"

/* -------------------------------------------------------------------------- */

template <class K> struct d_hash_traits;

template <class K, class V>
class d_hash 
{
public:
    typedef typename d_hash_traits<K>::Key Key;
    typedef typename d_hash_traits<K>::KeyRef KeyRef;

    struct iterator {
        iterator() : key(), value(*(V*) NULL) {}
        iterator(KeyRef k, V* v) : key(k), value(*v) {}

        KeyRef key;
        V& value;
    };

    struct const_iterator {
        const_iterator() : key(), value(*(const V*) NULL) {}
        const_iterator(KeyRef k, const V* v) : key(k), value(*v) {}

        KeyRef key;
        const V& value;
    };

    d_hash();
    d_hash(const d_hash<K,V>& r);
    d_hash<K,V>& operator=(const d_hash<K,V>& r);
    ~d_hash();

    void clear();
    size_t size() const {return m_Base.size;}
    bool empty() const {return size() == 0;}

    V& add(KeyRef key);

    V& get(KeyRef key) {V* v = find(key); return v ? *v : m_InvalidValue;}
    const V& get(KeyRef key) const {V* v = find(key); return v ? *v : m_InvalidValue;}

    V* find(KeyRef key);
    const V* find(KeyRef key) const {return ((d_hash<K,V>*) this)->find(key);}

    void remove(KeyRef key) {erase(find(key));}
    void erase(V& val) {erase(&val);}
    void erase(V* pval);

    bool has_next(iterator* iter);
    bool has_next(const_iterator* iter) const;

private:
    dh_base m_Base;
    Key* m_Keys;
    V* m_Values;
    V m_InvalidValue;
};

/* -------------------------------------------------------------------------- */

template <class K, class V>
d_hash<K,V>::d_hash() : m_Keys(NULL), m_Values(NULL), m_InvalidValue() {
    memset(&m_Base, 0, sizeof(m_Base));
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
d_hash<K,V>::d_hash(const d_hash<K,V>& r) : m_Keys(NULL), m_Values(NULL), m_InvalidValue() {
    memset(&m_Base, 0, sizeof(m_Base));
    *this = r;
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
d_hash<K,V>& d_hash<K,V>::operator=(const d_hash<K,V>& r) {

    if (this == &r) {
        int i = -1;
        while (dh_hasnext_base(&m_Base, &i)) {
            m_Keys[i].~Key();
            m_Values[i].~V();
        }
        dh_free_base(&m_Base, m_Keys, m_Values);

        size_t sz = r.m_Base.n_buckets;
        m_Keys = (Key*) malloc(sizeof(Key) * sz);
        m_Values = (V*) malloc(sizeof(V) * sz);
        m_Base.flags = (uint32_t*) malloc(sizeof(uint32_t) * (1 + (sz >> 4)));

        memcpy(m_Base.flags, r.m_Base.flags, sizeof(uint32_t) * (1 + (sz >> 4)));

        i = -1;
        while (dh_hasnext_base(&r.m_Base, &i)) {
            new (&m_Keys[i]) Key(r.m_Keys[i]);
            new (&m_Values[i]) V(r.m_Values[i]);
        }

    }

    return *this;
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
d_hash<K,V>::~d_hash() {
    int i = -1;
    while (dh_hasnext_base(&m_Base, &i)) {
        m_Keys[i].~Key();
        m_Values[i].~V();
    }
    dh_free_base(&m_Base, m_Keys, m_Values);
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
void d_hash<K,V>::clear() {
    int i = -1;
    while (dh_hasnext_base(&m_Base, &i)) {
        m_Keys[i].~Key();
        m_Values[i].~V();
    }
    dh_clear_base(&m_Base);
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
V& d_hash<K,V>::add(KeyRef key) {
    if (d_hash_traits<K>::add(&m_Base, m_Keys, key, sizeof(V))) {
        new (m_Values + m_Base.idx) V();
    }

    return m_Values[m_Base.idx];
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
V* d_hash<K,V>::find(KeyRef key) {
    if (d_hash_traits<K>::get(&m_Base, key)) {
        return m_Values + m_Base.idx;
    } else {
        return NULL;
    }
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
void d_hash<K,V>::erase(V* pval) {
    if (pval) {
        assert(m_Values <= pval && pval < m_Values + m_Base.n_buckets);
        int i = pval - m_Values;
        m_Keys[i].~Key();
        m_Values[i].~V();
        dh_erase_base(&m_Base, i);
    }
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
bool d_hash<K,V>::has_next(iterator* piter) {
    int i = &piter->value ? &piter->value - m_Values : -1;

    if (dh_hasnext_base(&m_Base, &i)) {
        new (piter) iterator((KeyRef) m_Keys[i], &m_Values[i]);
        return true;
    } else {
        new (piter) iterator();
        return false;
    }
}

/* -------------------------------------------------------------------------- */

template <class K, class V>
bool d_hash<K,V>::has_next(const_iterator* piter) const {
    int i = &piter->value ? &piter->value - m_Values : -1;

    if (dh_hasnext_base(&m_Base, &i)) {
        new (piter) const_iterator((KeyRef) m_Keys[i], &m_Values[i]);
        return true;
    } else {
        new (piter) const_iterator();
        return false;
    }
}

/* -------------------------------------------------------------------------- */

template <>
struct d_hash_traits<d_slice<char> > {
    typedef d_slice<char> KeyRef;
    typedef d_slice<char> Key;

    static bool get(dh_base* h, d_slice<char> key) {
        return dhs_get_base(h, key);
    }

    static bool add(dh_base* h, d_slice<char>* keys, d_slice<char> key, size_t valsz) {
        (void) keys;
        return dhs_add_base(h, key, valsz);
    }
};

template <>
struct d_hash_traits<d_vector<char> > {
    typedef d_slice<char> KeyRef;
    typedef d_vector<char> Key;

    static bool get(dh_base* h, d_slice<char> key) {
        return dhs_get_base(h, key);
    }

    static bool add(dh_base* h, d_vector<char>* keys, d_slice<char> key, size_t valsz) {
        if (dhs_add_base(h, key, valsz)) {
            new (keys + h->idx) d_vector<char>(key);
            return true;
        }

        return false;
    }
};

template <class K>
struct d_hash_traits {
    typedef K KeyRef;
    typedef intmax_t Key;

    static bool get(dh_base* h, intmax_t key) {
        return dhi_get_base(h, key);
    }

    static bool get(dh_base* h, void* key) {
        return dhi_get_base(h, (uintptr_t) key);
    }

    static bool add(dh_base* h, intmax_t* keys, intmax_t key, size_t valsz) {
        (void) keys;
        return dhi_add_base(h, key, valsz);
    }

    static bool add(dh_base* h, intmax_t* keys, void* key, size_t valsz) {
        (void) keys;
        return dhi_add_base(h, (uintptr_t) key, valsz);
    }
};

/* -------------------------------------------------------------------------- */

