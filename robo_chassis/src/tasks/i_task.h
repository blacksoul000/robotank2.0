#ifndef I_TASK_H
#define I_TASK_H

#include <QSharedPointer>

class ITask : public QObject
{
public:
    virtual ~ITask() = default;

    virtual void start() {}
    virtual void execute() {}

protected:
    ITask() = default;
};

using ITaskPtr = QSharedPointer< ITask >;

#endif // I_TASK_H
