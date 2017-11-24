#include "task_manager.h"
#include "thread_controller.h"

// tasks
#include "gpio_controller.h"
#include "robo_core.h"
#include "arduino_exchanger.h"
#include "gamepad_controller.h"
#include "tracker_task.h"
#include "video_source.h"
#include "gui_exchanger.h"
#include "config_handler.h"

// Qt
#include <QMap>
#include <QThread>
#include <QDebug>

class TaskManager::Impl
{
public:
    int& argc;
    char** argv;

    QMap< quint64, ThreadControllerPtr > threads;
    QList < ITaskPtr > untreaded;

    Impl(int& argc, char** argv) : argc(argc), argv(argv) {}
};

TaskManager::TaskManager(int& argc, char** argv) :
    d(new Impl(argc, argv))
{
}

TaskManager::~TaskManager()
{
    delete d;
}

bool TaskManager::createTasks()
{
    this->addTask(ITaskPtr(new RoboCore), 30);
    this->addTask(ITaskPtr(new ArduinoExchanger), 0);
    this->addTask(ITaskPtr(new TrackerTask), 60);
    this->addTask(ITaskPtr(new GamepadController), 30);
    this->addTask(ITaskPtr(new GuiExchanger), 40);
    this->addTask(ITaskPtr(new VideoSource(d->argc, d->argv)), 100);
    this->addTask(ITaskPtr(new GpioController), 10);
    this->addTask(ITaskPtr(new ConfigHandler), 0); // last one
}

bool TaskManager::addTask(const ITaskPtr& task, quint64 delay)
{
    if (delay == 0)
    {
        d->untreaded.append(task);
        return true;
    }
    ThreadControllerPtr thread;
    if (d->threads.contains(delay))
    {
        thread = d->threads.value(delay);
    }
    else
    {
        thread = ThreadControllerPtr::create(delay);
        d->threads.insert(delay, thread);
    }
    thread->addTask(task);
    return true;
}

void TaskManager::start()
{
    for(const auto& task: d->untreaded)
    {
        task->start();
    }

    for(const auto& thread: d->threads)
    {
        thread->start();
    }
}
