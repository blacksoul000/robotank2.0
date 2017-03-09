#include "task_worker.h"

#include <QList>
#include <QElapsedTimer>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>

class TaskWorker::Impl
{
public:
    QList < ITaskPtr > tasks;
    QElapsedTimer timer;
    quint64 interval;
};

TaskWorker::TaskWorker(quint64 interval, QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    d->interval = interval;
}

TaskWorker::~TaskWorker()
{
    delete d;
}

void TaskWorker::addTask(const ITaskPtr& task)
{
    d->tasks.append(task);
}

void TaskWorker::start()
{
    for(const auto& task: d->tasks)
    {
        task->start();
    }

    while (1)
    {
        d->timer.start();
        for(const auto& task: d->tasks)
        {
            task->execute();
        }
        qApp->processEvents();
        auto elapsed = d->timer.elapsed();
        qint64 sleep = d->interval - elapsed;
        if (sleep > 0) QThread::currentThread()->msleep(sleep);
    }
}
