#include "task_manager.h"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("RoboChassis");
    TaskManager manager(argc, argv);
    manager.createTasks();
    manager.start();
    return app.exec();
}

