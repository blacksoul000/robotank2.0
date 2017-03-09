#ifndef PUBLISHER_H
#define PUBLISHER_H

#include "subscriber.h"
#include "call_event.h"

#include <QCoreApplication>
#include <QThread>
#include <QList>
#include <QDebug>

template < class T >
class Publisher
{
public:
    Publisher() = default;

    void publish(const T& msg)
    {
        for(const auto& sub: m_subscribers)
        {
            if (QThread::currentThread() == sub->thread())
            {
                sub->call(msg);
            }
            else
            {
                qApp->postEvent(sub, new CallEvent< T >(msg));
            }
        }
    }

    void subscribe(Subscriber< T >* sub)
    {
        m_subscribers.append(sub);
    }

private:
    QList< Subscriber< T >* > m_subscribers;
};

#endif // PUBLISHER_H
