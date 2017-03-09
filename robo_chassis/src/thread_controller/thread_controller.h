#ifndef THREADCONTROLLER_H
#define THREADCONTROLLER_H

#include "i_task.h"

#include <QObject>
#include <QSharedPointer>

class ThreadController : public QObject
{
public:
    explicit ThreadController(quint64 interval, QObject* parent = 0);
    ~ThreadController();

    void addTask(const ITaskPtr& task);

    void start();

private:
    class Impl;
    Impl* d;
};

using ThreadControllerPtr = QSharedPointer< ThreadController >;

#endif // THREADCONTROLLER_H
