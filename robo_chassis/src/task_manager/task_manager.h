#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "i_task.h"

class TaskManager
{
public:
    TaskManager(int &argc, char **argv);
    ~TaskManager();

    bool createTasks();

    bool addTask(const ITaskPtr& task, quint64 delay);
    void start();

private:
    class Impl;
    Impl* d;
};

#endif // TASKMANAGER_H
