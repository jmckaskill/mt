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

#include <mt/message.h>
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QCoreApplication>

namespace MT
{
    // Use a global of a dummy template to get a weakly linked global variable
    namespace detail {

        template <class Tag = void>
        struct EventType {
            static QEvent::Type type;
        };

        EventType<Tag>::type = (QEvent::Type) QEvent::registerEventType();
    }

class QtMessageQueue : public QObject
{
public:
    QtMessageQueue() {
        m_EventType = detail::EventType<>::type;
        m_Queue = MT_NewMessageQueue();
        MT_SetMessageQueueWakeup(m_Queue, BindVoid(&PostWakeup, this));
    }

    ~QtMessageQueue() {
        MT_FreeMessageQueue(m_Queue);
    }

    virtual bool event(QEvent* e) {
        if (e->type == m_EventType) {
            MT_ProcessMessageQueue(m_Queue);
            return true;
        } else {
            return QObject::event(e);
        }
    }

    void setCurrent() {
        MT_SetCurrentMessageQueue(m_Queue);
    }

    MT_MessageQueue* queue() {
        return m_Queue;
    }

private:
    static void PostWakeup(QtMessageQueue* s) {
      QCoreApplication::postEvent(s, new QEvent(m_EventType));
    }

    QEvent::Type m_EventType;
    MT_MessageQueue* m_Queue;
};

}


