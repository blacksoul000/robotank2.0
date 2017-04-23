#ifndef TASKWORKER_H
#define TASKWORKER_H

#include "i_task.h"

#include <QObject>

class TaskWorker : public QObject
{
public:
    explicit TaskWorker(quint64 interval, QObject *parent = 0);
    ~TaskWorker();

    void addTask(const ITaskPtr& task);

    void start();
    void stop();

private:
    class Impl;
    Impl* d;
};

#endif // TASKWORKER_H
