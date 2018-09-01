#include "task_manager.h"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("RoboChassis");
    TaskManager manager;
    manager.createTasks();
    manager.start();
    return app.exec();
}

