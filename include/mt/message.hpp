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

#include <mt/message.h>
#include <delegate.h>
#include <new>

namespace MT
{

/* ------------------------------------------------------------------------- */

class Object : public MT_Object
{
    MT_NOT_COPYABLE(Object);
public:
    Object() {
        MT_InitObject(this);
    }

    ~Object() {
        MT_DestroyObject(this);
    }
};

/* ------------------------------------------------------------------------- */

template <class Data>
class Pipe
{
public:
    Pipe() {
        MT_InitPipe(Data, &d);
    }

    Pipe(const Pipe<Data>& r) {
        MT_InitPipe(Data, &d);
        MT_CopyPipe(&d, r.d);
    }

    ~Pipe() {
        MT_DestroyPipe(&d);
    }

    Pipe<Data>& operator=(const Pipe<Data>& r) {
        MT_CopyPipe(&d, r.d);
        return *this;
    }

    void SendDirect(const Data* data) const {
        MT_SendDirect(&d, data);
    }

    void SendDirect(const Data& data) const {
        MT_SendDirect(&d, &data);
    }

    void SendProxied(const Data* data) const {
        MT_SendProxied(&d, data);
    }

    void SendProxied(const Data& data) const {
        MT_SendProxied(&d, &data);
    }

    void operator() (const Data& data) const {
        MT_Send(&d, &data);
    }

    void operator() (const Data* data) const {
        MT_Send(&d, data);
    }

/*private:*/

    static void Init(Data* to, const Data* from) {
        new (to) Data(*from);
    }

    static void Destroy(Data* p) {
        p->Data::~Data();
    }

    MT_DECLARE_MESSAGE_TYPE(Data, Data, &Init, &Destroy);

    MT_Pipe(Data) d;
};

template <class T, class M, class Data>
Pipe<Data> MakePipe(void (M::*mf)(const Data*), T* p)
{
    Pipe<Data> ret;
    ret.d.dlg = Pipe<Data>::Bind_MT_Delegate_Data(mf, p);
    ret.d.weak_data = MT_GetWeakData(p);
    MT_RefWeakData(ret.d.weak_data);
    return ret;
}

template <class T, class M, class Data>
Pipe<Data> MakePipe(void (M::*mf)(const Data*) const, const T* p)
{
    Pipe<Data> ret;
    ret.d.dlg = Pipe<Data>::Bind_MT_Delegate_Data(mf, p);
    ret.d.weak_data = MT_GetWeakData(p);
    MT_RefWeakData(ret.d.weak_data);
    return ret;
}

template <class T, class M, class Data>
Pipe<Data> MakePipe(void (M::*mf)(const Data&), T* p)
{
    Pipe<Data> ret;
    ret.d.dlg = Pipe<Data>::Bind_MT_Delegate_Data((void (M::*)(const Data*)) mf, p);
    ret.d.weak_data = MT_GetWeakData(p);
    MT_RefWeakData(ret.d.weak_data);
    return ret;
}

template <class T, class M, class Data>
Pipe<Data> MakePipe(void (M::*mf)(const Data&) const, const T* p)
{
    Pipe<Data> ret;
    ret.d.dlg = Pipe<Data>::Bind_MT_Delegate_Data((void (M::*)(const Data*) const) mf, p);
    ret.d.weak_data = MT_GetWeakData(p);
    MT_RefWeakData(ret.d.weak_data);
    return ret;
}

/* ------------------------------------------------------------------------- */

template <class Data>
class Signal
{
public:
    Signal() {
        MT_InitSignal(Data, &d);
    }

    ~Signal() {
        MT_DestroySignal(&d);
    }

    void Connect(const Pipe<Data>& ch, MT_SendType type = MT_SEND_AUTO) {
        MT_Connect3(&d, &ch.d, type);
    }

    template <class MF, class T>
    void Connect(MF mf, T p, MT_SendType type = MT_SEND_AUTO) {
        Pipe<Data> pipe = MakePipe(mf, p);
        MT_Connect3(&d, &pipe.d, type);
    }

    void operator += (const Pipe<Data>& ch) {
        MT_Connect2(&d, &ch.d);
    }

    void Emit(const Data* p) {
        MT_Emit(&d, p);
    }

    void Emit(const Data& p) {
        MT_Emit(&d, &p);
    }

    void operator() (const Data* p) {
        MT_Emit(&d, p);
    }

    void operator() (const Data& p) {
        MT_Emit(&d, &p);
    }

/*private:*/

    static void Init(Data* to, const Data* from) {
        new (to) Data(*from);
    }

    static void Destroy(Data* p) {
        p->Data::~Data();
    }

    MT_DECLARE_MESSAGE_TYPE(Data, Data, &Init, &Destroy);

    MT_Signal(Data) d;
};

/* ------------------------------------------------------------------------- */

}

