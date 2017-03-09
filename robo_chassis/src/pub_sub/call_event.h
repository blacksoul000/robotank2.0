#ifndef CALL_EVENT_H
#define CALL_EVENT_H

#include <QEvent>

template < class T >
class CallEvent : public QEvent
{
public:
    static const QEvent::Type CallBack = static_cast<QEvent::Type>(6843);
    CallEvent(const T& data, Type type = CallBack) : QEvent(type), m_data(data) {}

    inline const T& data() const { return m_data; }

private:
    T m_data;
};

#endif // CALL_EVENT_H
