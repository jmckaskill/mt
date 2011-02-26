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

#include "vector.h"
#include "char.h"
#include <assert.h>
#include <new>

template <class T> struct d_vector;
template <class T> struct d_slice;
template <class T> struct d_vector_traits;

/* -------------------------------------------------------------------------- */

namespace d_detail {
template <class T> d_slice<T> left(const T* p, int size, int off);
template <class T> d_slice<T> right(const T* p, int size, int off);
template <class T> d_slice<T> slice(const T* p, int size, int off, int num);
}

/* -------------------------------------------------------------------------- */

template <>
class d_slice<uint8_t> 
{
public:
    d_slice() : m_Size(0), m_Data(NULL) {}
    d_slice(const char* str) : m_Size((int) strlen(str)), m_Data((uint8_t*) str) {}
    d_slice(const char* d, int sz) : m_Size(sz), m_Data((uint8_t*) d) {}
    d_slice(const char* d, size_t sz) : m_Size((int) sz), m_Data((uint8_t*) d) {}
    d_slice(const char* d, ptrdiff_t sz) : m_Size((int) sz), m_Data((uint8_t*) d) {}
    d_slice(const uint8_t* d, int sz) : m_Size(sz), m_Data(d) {}
    d_slice(const uint8_t* d, size_t sz) : m_Size((int) sz), m_Data(d) {}
    d_slice(const uint8_t* d, ptrdiff_t sz) : m_Size((int) sz), m_Data(d) {}
    template <int sz> d_slice(const char (&d)[sz]) : m_Size(sz), m_Data((uint8_t*) &d[0]) {}
    template <int sz> d_slice(const uint8_t (&d)[sz]) : m_Size(sz), m_Data(&d[0]) {}
    d_slice(d_Slice(char) r) : m_Size(r.size), m_Data((uint8_t*) r.data) {}

    d_slice(d_slice<char> r);
    d_slice(const d_vector<char>& r);
    d_slice(const d_vector<uint8_t>& r);

    int size() const {return m_Size;}
    const uint8_t* data() const {return m_Data;}
    const uint8_t* end() const {return m_Data + m_Size;}
    bool empty() const {return m_Size == 0;}
    const uint8_t& operator[] (int i) const {return m_Data[i];}

    int find(uint8_t val) {
        uint8_t* p = (uint8_t*) memchr(m_Data, val, m_Size);
        return p ? p - m_Data : -1;
    }

    int find(d_slice<uint8_t> val) {
        uint8_t* p = (uint8_t*) memmem(m_Data, m_Size, val.m_Data, val.m_Size);
        return p ? p - m_Data : -1;
    }

    int find_last(uint8_t val) {
        uint8_t* p = (uint8_t*) memrchr(m_Data, val, m_Size);
        return p ? p - m_Data : -1;
    }

    int find_last(d_slice<uint8_t> val) {
        uint8_t* p = (uint8_t*) memrmem(m_Data, m_Size, val.m_Data, val.m_Size);
        return p ? p - m_Data : -1;
    }

    bool begins_with(d_slice<uint8_t> r) const  {return m_Size >= r.size() && 0 == memcmp(m_Data, r.m_Data, r.m_Size);}
    bool ends_with(d_slice<uint8_t> r) const    {return m_Size >= r.size() && 0 == memcmp(m_Data + m_Size - r.m_Size, r.m_Data, r.m_Size);}
    bool operator == (d_slice<uint8_t> r) const {return m_Size == r.m_Size && 0 == memcmp(m_Data, r.m_Data, m_Size);}
    bool operator != (d_slice<uint8_t> r) const {return m_Size != r.m_Size || 0 != memcmp(m_Data, r.m_Data, m_Size);}
    bool operator < (d_slice<uint8_t> r) const  {return compare(r) < 0;} 
    bool operator <= (d_slice<uint8_t> r) const {return compare(r) <= 0;}
    bool operator >= (d_slice<uint8_t> r) const {return compare(r) >= 0;}
    bool operator > (d_slice<uint8_t> r) const  {return compare(r) > 0;}

    d_vector<uint8_t> operator+(d_slice<uint8_t> r) const;

    template <typename size>
    d_slice<uint8_t> left(size off) const {return d_detail::left(m_Data, m_Size, (int) off);}

    template <typename size>
    d_slice<uint8_t> right(size off) const {return d_detail::right(m_Data, m_Size, (int) off);}

    template <typename size1, typename size2>
    d_slice<uint8_t> slice(size1 off, size2 num) const {return d_detail::slice(m_Data, m_Size, (int) off, (int) num);}

private:
    int compare(d_slice<uint8_t> r) const {
        int sz = m_Size < r.m_Size ? m_Size : r.m_Size;
        int ret = memcmp(m_Data, r.m_Data, sz);
        return ret != 0 ? ret : r.m_Size - m_Size;
    }

    int m_Size;
    const uint8_t* m_Data;
};

/* -------------------------------------------------------------------------- */

template <>
class d_slice<char> : public d_Slice(char)
{
public:
    typedef d_Slice(char) base;

    d_slice() {init(NULL, 0);}
    d_slice(char* str) {init(str, strlen(str));}
    d_slice(const char* str) {init(str, strlen(str));}
    d_slice(const char* d, int sz) {init(d, sz);}
    d_slice(const char* d, size_t sz) {init(d, sz);}
    template <int sz> d_slice(const char (&d)[sz]) {init(&d[0], sz);}
    d_slice(d_Slice(char) r) {init(r.data, r.size);}

    d_slice(const d_vector<char>& r);
    explicit d_slice(d_slice<uint8_t> r);

    void init(const char* str, size_t sz) {
        base::data = str;
        base::size = sz;
    }

    int size() const {return base::size;}
    const char* data() const {return base::data;}
    const char* end() const {return base::data + base::size;}
    bool empty() const {return base::size == 0;}
    const char& operator[] (int i) const {return base::data[i];}

    int find(char val) const                {return d_slice<uint8_t>(*this).find(val);}
    int find(d_slice<char> val) const       {return d_slice<uint8_t>(*this).find(val);}
    int find_last(char val) const           {return d_slice<uint8_t>(*this).find_last(val);}
    int find_last(d_slice<char> val) const  {return d_slice<uint8_t>(*this).find_last(val);}

    bool begins_with(d_slice<char> r) const {return d_slice<uint8_t>(*this).begins_with(r);}
    bool ends_with(d_slice<char> r) const   {return d_slice<uint8_t>(*this).ends_with(r);}
    bool operator == (d_slice<char> r) const{return d_slice<uint8_t>(*this) == r;}
    bool operator != (d_slice<char> r) const{return d_slice<uint8_t>(*this) != r;}
    bool operator < (d_slice<char> r) const {return d_slice<uint8_t>(*this) < r;}
    bool operator <= (d_slice<char> r) const{return d_slice<uint8_t>(*this) <= r;}
    bool operator >= (d_slice<char> r) const{return d_slice<uint8_t>(*this) >= r;}
    bool operator > (d_slice<char> r) const {return d_slice<uint8_t>(*this) > r;}

    d_vector<char> operator+(d_slice<char> r) const;

    template <typename size>
    d_slice<char> left(size off) const {return d_detail::left(base::data, base::size, (int) off);}

    template <typename size>
    d_slice<char> right(size off) const {return d_detail::right(base::data, base::size, (int) off);}

    template <typename size1, typename size2>
    d_slice<char> slice(size1 off, size2 num) const {return d_detail::slice(base::data, base::size, (int) off, (int) num);}
};

/* -------------------------------------------------------------------------- */

template <class T>
class d_slice
{
public:
    d_slice() : m_Size(0), m_Data(NULL) {}
    d_slice(const T* d, int sz) : m_Size(sz), m_Data(d) {}
    d_slice(const T* d, size_t sz) : m_Size(sz), m_Data(d) {}
    template <int sz> d_slice(const T (&d)[sz]) : m_Size(sz), m_Data(&d[0]) {}
    d_slice(const d_vector<T>& r) : m_Size(r.size()), m_Data(r.data()) {}

    int size() const {return m_Size;}
    const T* data() const {return m_Data;}
    const T* end() const {return m_Data + m_Size;}
    bool empty() const {return m_Size == 0;}
    const T& operator[] (int i) const {return m_Data[i];}

    template <class U>
    int find(const U& val) {
        for (int i = 0; i < m_Size; i++) {
            if (m_Data[i] == val) {
                return i;
            }
        }
        return -1;
    }

    template <class U>
    int find_last(const U& val) {
        for (int i = m_Size - 1; i >= 0; i++) {
            if (m_Data[i] == val) {
                return i;
            }
        }
        return -1;
    }

    template <typename size>
    d_slice<T> left(size off) const {return d_detail::left(m_Data, m_Size, (int) off);}

    template <typename size>
    d_slice<T> right(size off) const {return d_detail::right(m_Data, m_Size, (int) off);}

    template <typename size1, typename size2>
    d_slice<T> slice(size1 off, size2 num) const {return d_detail::slice(m_Data, m_Size, (int) off, (int) num);}

private:
    int m_Size;
    const T* m_Data;
};


/* -------------------------------------------------------------------------- */

template <>
struct d_vector_traits<char> {
    static void construct(char* p, int num) {(void) p; (void) num;}
    static void destruct(char* p, int num) {(void) p; (void) num;}
    static void copy(char* p, const char* from, int num) {memcpy(p, from, num);}
    typedef d_Vector(char) base_vector;
};

template <>
struct d_vector_traits<uint8_t> {
    static void construct(uint8_t* p, int num) {(void) p; (void) num;}
    static void destruct(uint8_t* p, int num) {(void) p; (void) num;}
    static void copy(uint8_t* p, const uint8_t* from, int num) {memcpy(p, from, num);}

    struct base_vector {
        int size;
        uint8_t* data;
    };
};

/* -------------------------------------------------------------------------- */

template <class T>
struct d_vector_traits {
    static void construct(T* p, int num) {
        for (int i= 0; i < num; i++) {
            new (p+i) T();
        }
    }

    static void destruct(T* p, int num) {
        for (int i= 0; i < num; i++) {
            p[i].~T();
        }
    }

    static void copy(T* p, const T* from, int num) {
        for (int i = 0; i < num; i++) {
            new (p+i) T(from[i]);
        }
    }

    struct base_vector {
        int size;
        T* data;
    };
};

/* -------------------------------------------------------------------------- */

template <class T>
struct d_vector : d_vector_traits<T>::base_vector {

    typedef typename d_vector_traits<T>::base_vector base;

    d_vector() {init();}
    d_vector(d_slice<T> r) {init(); append(r);}
    d_vector(const T* str) {init(); append(str);}
    d_vector(const T* d, int sz) {init(); append(d, sz);}
    d_vector(const T* d, size_t sz) {init(); append(d, sz);}
    template <int sz> d_vector(const T (&d)[sz]) {init(); append(d);}
    d_vector(const d_vector<T>& r) {init(); append(r);}

    void init() {
        base::data = NULL;
        base::size = 0;
    }

    d_vector<T>& operator=(const d_vector<T>& r) {
        set(r);
        return *this;
    }

    d_vector<T>& operator=(d_slice<T> r) {
        set(r);
        return *this;
    }

    d_vector<T>& operator=(const char* str) {
        set(str);
        return *this;
    }

    template <int sz>
    d_vector<T>& operator=(const T (&d)[sz]) {
        set(&d[0], sz);
        return *this;
    }

    d_vector<T>& operator+=(d_slice<T> r) {
        append(r);
        return *this;
    }

    d_vector<T> operator+(d_slice<T> r) const {
        d_vector<T> copy = *this;
        return copy += r;
    }

    ~d_vector() {
        clear();
        dv_free_base(base::data);
    }

    void resize(int sz) {
        if (sz > base::size) {
            base::data = (T*) dv_resize_base(base::data, sizeof(T) * sz);
            d_vector_traits<T>::construct(base::data + base::size, sz - base::size);
        } else {
            d_vector_traits<T>::destruct(base::data + sz, base::size - sz);
            base::data = (T*) dv_resize_base(base::data, sizeof(T) * sz);
        }

        base::size = sz;
    }

    void set(d_slice<T> v) {
        // v may be a slice of this vector so copy first
        int sz = base::size;
        append(v);
        erase(0, sz);
    }

    void append(d_slice<T> v) {
        int newsz = base::size + v.size();
        base::data = (T*) dv_resize_base(base::data, sizeof(T) * newsz);
        d_vector_traits<T>::copy(base::data + base::size, v.data(), v.size());
        base::size = newsz;
    }

    void insert(int i, d_slice<T> v) {
        int newsz = base::size + v.size();
        base::data = (T*) dv_resize_base(base::data, sizeof(T) * newsz);
        memmove(base::data + i + v.size, base::data + i, (base::size - i) * sizeof(T));
        d_vector_traits<T>::copy(base::data + i, v.data(), v.size());
        base::size = newsz;
    }

    T* append_buffer(int num) {
        int newsz = base::size + num;
        base::data = (T*) dv_resize_base(base::data, sizeof(T) * newsz);
        T* ret = base::data + base::size;
        d_vector_traits<T>::construct(ret, num);
        base::size = newsz;
        return ret;
    }

    T* insert_buffer(int i, int num) {
        int newsz = base::size + num;
        base::data = (T*) dv_resize_base(base::data, sizeof(T) * newsz);
        memmove(base::data + i + num, base::data + i, (base::size - i) * sizeof(T));
        T* ret = base::data + i;
        d_vector_traits<T>::construct(ret, num);
        base::size = newsz;
        return ret;
    }

    void erase(int off, int num) {
        int newsz = base::size - num;
        d_vector_traits<T>::destruct(base::data + off, num);
        base::data = (T*) dv_resize_base(base::data, sizeof(T) * newsz);
        base::size = newsz;
    }

    int print(const T* format, ...) {
        va_list ap;
        va_start(ap, format);
        return dv_vprint((base*) this, format, ap);
    }

    int vprint(const T* format, va_list ap) {
        return dv_vprint((base*) this, format, ap);
    }

    void set(const T* d, int sz)                {set(d_slice<T>(d, sz));}
    void set(const T* d, size_t sz)             {set(d_slice<T>(d, sz));}
    void append(const T& d)                     {append(d_slice<T>(&d, 1));}
    void append(const T* d, int sz)             {append(d_slice<T>(d, sz));}
    void append(const T* d, size_t sz)          {append(d_slice<T>(d, sz));}
    void insert(int i, const T& d)              {insert(i, d_slice<T>(&d, 1));}
    void insert(int i, const T* d, int sz)      {insert(i, d_slice<T>(d, sz));}
    void insert(int i, const T* d, size_t sz)   {insert(i, d_slice<T>(d, sz));}

    T* append_buffer(size_t sz)                 {return append_buffer((int) sz);}
    T* insert_buffer(int off, size_t num)       {return insert_buffer((int) off, (int) num);}
    T* insert_buffer(size_t off, int num)       {return insert_buffer((int) off, (int) num);}
    T* insert_buffer(size_t off, size_t num)    {return insert_buffer((int) off, (int) num);}

    void clear()                                {erase(0, base::size);}

    void erase(int off, size_t num)             {erase(off, (int) num);}
    void erase(size_t off, size_t num)          {erase((int) off, (int) num);}

    void erase_end(int num)                     {erase(base::size - num, num);}
    void erase_end(size_t num)                  {erase_end((int) num);}

    template <class U>
    void remove_first(const U& val) {
        int i = find(val);
        if (i > 0) erase(i, 1);
    }

    template <class U>
    void remove_all(const U& val) {
        for (int i = 0; i < base::size;) {
            if (base::data[i] == val) {
                erase(i, 1);
            } else {
                i++;
            }
        }
    }

    template <class U>
    int find(const U& val)                      {return d_slice<T>(*this).find(val);}

    template <class U>
    int find_last(const T& val)                 {return d_slice<T>(*this).find(val);}

    d_slice<T> left(int off) const              {return d_slice<T>(*this).left(off);}
    d_slice<T> left(size_t off) const           {return d_slice<T>(*this).left(off);}
    d_slice<T> right(int off) const             {return d_slice<T>(*this).right(off);}
    d_slice<T> right(size_t off) const          {return d_slice<T>(*this).right(off);}
    d_slice<T> slice(int off, int num) const    {return d_slice<T>(*this).slice(off, num);}
    d_slice<T> slice(size_t off, int num) const {return d_slice<T>(*this).slice(off, num);}
    d_slice<T> slice(int off, size_t num) const {return d_slice<T>(*this).slice(off, num);}
    d_slice<T> slice(size_t off, size_t num) const {return d_slice<T>(*this).slice(off, num);}

    d_slice<uint8_t> memblock() const;

    int size() const {return base::size;}
    T* data() {return base::data;}
    T* end() {return base::data + base::size;}
    const T* data() const {return base::data;}
    const T* end() const {return base::data + base::size;}
    const T* c_str() const {return base::data;}
    bool empty() const {return base::size == 0;}

    T& operator[] (int i) {return base::data[i];}
    const T& operator[] (int i) const {return base::data[i];}

    bool begins_with(d_slice<T> r) const  {return d_slice<T>(*this).begins_with(r);}
    bool ends_with(d_slice<T> r) const    {return d_slice<T>(*this).ends_with(r);}
    bool operator == (d_slice<T> r) const {return d_slice<T>(*this) == r;}
    bool operator != (d_slice<T> r) const {return d_slice<T>(*this) != r;}
    bool operator <= (d_slice<T> r) const {return d_slice<T>(*this) <= r;}
    bool operator >= (d_slice<T> r) const {return d_slice<T>(*this) >= r;}
    bool operator < (d_slice<T> r) const {return d_slice<T>(*this) < r;}
    bool operator > (d_slice<T> r) const {return d_slice<T>(*this) > r;}
};

/* -------------------------------------------------------------------------- */

inline d_slice<uint8_t>::d_slice(d_slice<char> r)
: m_Size(r.size()), m_Data((uint8_t*) r.data()) {}

inline d_slice<uint8_t>::d_slice(const d_vector<char>& r)
: m_Size(r.size()), m_Data((uint8_t*) r.data()) {}

inline d_slice<uint8_t>::d_slice(const d_vector<uint8_t>& r)
: m_Size(r.size()), m_Data(r.data()) {}

inline d_slice<char>::d_slice(d_slice<uint8_t> r)
{init((char*) r.data(), r.size());}

inline d_slice<char>::d_slice(const d_vector<char>& r)
{init(r.data(), r.size());}

inline d_vector<char> d_slice<char>::operator+(d_slice<char> r) const {
    d_vector<char> ret = *this;
    ret += r;
    return ret;
}

/* -------------------------------------------------------------------------- */

template <class T> d_slice<T> d_detail::left(const T* p, int size, int off) {
    if (off >= 0) {
        assert(off <= size);
        return d_slice<T>(p, off);
    } else {
        assert(off >= -size);
        return d_slice<T>(p, size + off);
    }
}

template <class T> d_slice<T> d_detail::right(const T* p, int size, int off) {
    if (off >= 0) {
        assert(off <= size);
        return d_slice<T>(p + off, size - off);
    } else {
        assert(off >= -size);
        return d_slice<T>(p + size + off, -off);
    }
}

template <class T> d_slice<T> d_detail::slice(const T* p, int size, int off, int num) {
    assert(num >= 0);
    if (off >= 0) {
        assert(off + num <= size);
        return d_slice<T>(p + off, num);
    } else {
        assert(size + off + num <= size);
        return d_slice<T>(p + size + off, num);
    }
}

/* -------------------------------------------------------------------------- */

template <class T>
d_slice<uint8_t> d_vector<T>::memblock() const {
    return d_slice<uint8_t>((uint8_t*) base::data, base::size * sizeof(T));
}

/* -------------------------------------------------------------------------- */

inline d_vector<char> d_print(const char* format, ...) {
    d_vector<char> ret;
    va_list ap;
    va_start(ap, format);
    ret.vprint(format, ap);
    return ret;
}

template <class T>
inline d_slice<uint8_t> d_memblock(const T& d) {
    return d_slice<uint8_t>((uint8_t*) &d, sizeof(T));
}

template <class T>
T* d_memcast(d_slice<uint8_t> d) {
    return d.size() >= (int) sizeof(T) ? (T*) d.data() : NULL;
}

template <int sz>
void d_to_strbuf(char (&to)[sz], d_slice<char> from) {
    if (sz > from.size()) {
        memcpy(&to[0], &from[0], from.size());
        to[from.size()] = '\0';
    } else {
        memcpy(&to[0], &from[0], sz);
    }
}

template <int sz>
d_slice<char> d_from_strbuf(const char (&from)[sz]) {
    d_slice<char> str = from;

    int null = str.find('\0');
    if (null >= 0) {
        str = str.left(null);
    }

    return str;
}

#define D_PRI(slice) (slice).size(), (slice).data()

