#include "task_manager.h"
#include "thread_controller.h"

// tasks
#ifdef WITH_GPIO
#include "gpio_controller.h"
#endif  // WITH_GPIO
#include "robo_core.h"
#include "arduino_exchanger.h"
#include "gamepad_controller.h"
#ifdef WITH_TRACKING
#include "tracker_task.h"
#endif  // WITH_TRACKING
#include "video_source.h"
#include "bluetooth_manager.h"
#include "mavlink_exchanger.h"
#include "config_handler.h"

// Qt
#include <QMap>
#include <QThread>
#include <QDebug>

class TaskManager::Impl
{
public:
    QMap< quint64, ThreadControllerPtr > threads;
    QList < ITaskPtr > untreaded;
};

TaskManager::TaskManager() :
    d(new Impl)
{
}

TaskManager::~TaskManager()
{
    delete d;
}

bool TaskManager::createTasks()
{
    this->addTask(ITaskPtr(new GamepadController), 30);
    this->addTask(ITaskPtr(new RoboCore), 10);
#ifdef WITH_GPIO
    this->addTask(ITaskPtr(new GpioController), 20);
#endif  // WITH_GPIO
    this->addTask(ITaskPtr(new ArduinoExchanger), 20);
    this->addTask(ITaskPtr(new BluetoothManager), 2);
    this->addTask(ITaskPtr(new VideoSource), 100);
#ifdef WITH_TRACKING
    this->addTask(ITaskPtr(new TrackerTask), 100);
#endif  // WITH_TRACKING
    this->addTask(ITaskPtr(new MavlinkExchanger), 0);
    this->addTask(ITaskPtr(new ConfigHandler), 0); // last one
    return true;
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
