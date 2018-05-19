#include "thread_controller.h"
#include "task_worker.h"

#include <QThread>
#include <QDebug>

class ThreadController::Impl
{
public:
    QThread* thread = nullptr;
    TaskWorker* worker = nullptr;
};

ThreadController::ThreadController(quint64 interval, QObject* parent) :
    QObject(parent),
    d(new Impl)
{
    d->thread = new QThread;
    d->thread->setObjectName(QString("robo_%1").arg(interval));
    d->worker = new TaskWorker(interval);
    d->worker->moveToThread(d->thread);
}

ThreadController::~ThreadController()
{
    d->worker->stop();
    d->thread->quit();
    d->thread->wait();

    delete d->worker;
    delete d->thread;
    delete d;
}

void ThreadController::addTask(const ITaskPtr& task)
{
    task->moveToThread(d->thread);
    d->worker->addTask(task);
}

void ThreadController::start()
{
    connect(d->thread, &QThread::started, d->worker, &TaskWorker::start);
    d->thread->start();
}
