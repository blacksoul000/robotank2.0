#ifndef PUBSUB_H
#define PUBSUB_H

#include "publisher.h"
#include "subscriber.h"

#include <QHash>
#include <QDebug>
#include <QMutexLocker>
#include <QObject>

#include <functional>

class PubSub
{
public:

    static PubSub* instance()
    {
        static PubSub self;
        return &self;
    }

    template < typename T >
    Publisher < T >* advertise(const QString& topic)
    {
        QMutexLocker locker(&m_mutex);
        Publisher < T >* pub = new Publisher < T >;
        const auto& subscribers = m_subscribers.values(topic);

        for (void* ptr: subscribers)
        {
            auto sub = static_cast< Subscriber< T >* >(ptr);
            pub->subscribe(sub);
        }
        m_publishers.insert(topic, pub);
        return pub;
    }

    template < typename M, class T >
    void subscribe(const QString& topic, void(T::*fp)(const M&), T* object)
    {
        QMutexLocker locker(&m_mutex);
        auto sub = new Subscriber < M >(std::bind(fp, object, std::placeholders::_1), object);
        sub->moveToThread(object->thread());
        const auto& publishers = m_publishers.values(topic);
        for (auto ptr: publishers)
        {
            auto pub = static_cast< Publisher < M >* const >(ptr);
            pub->subscribe(sub);
        }
        m_subscribers.insert(topic, sub);
    }

private:
    PubSub() = default;
    ~PubSub() = default;

    QMutex m_mutex;
    QMultiHash< QString, void* > m_publishers;
    QMultiHash< QString, void* > m_subscribers;
};

#endif // PUBSUB_H
