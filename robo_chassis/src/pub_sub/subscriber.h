#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "call_event.h"

#include <QObject>
#include <QDebug>

#include <functional>

template < typename T >
class Subscriber : public QObject
{
public:
    Subscriber(const std::function< void(const T&) >& f)
    {
        m_callback = f;
    }

    void call(const T& value)
    {
        m_callback(value);
    }

protected:
    bool event(QEvent* e) override
    {
        if (e->type() == CallEvent< T >::CallBack)
        {
            auto event = dynamic_cast< CallEvent< T >* >(e);
            if (event)
            {
                this->call(event->data());
                event->accept();
                return true;
            }
        }
        return QObject::event(e);
    }

private:
    std::function< void(const T&) > m_callback;
};

#endif // SUBSCRIBER_H
