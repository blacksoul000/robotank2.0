#ifndef TRACKER_TASK_H
#define TRACKER_TASK_H

#include "i_task.h"

#include <QSharedPointer>

namespace cv
{
    class Mat;
}

class TrackerTask : public ITask
{
public:
    TrackerTask();
    ~TrackerTask();

    void execute() override;

    void onToggleRequest(const QRectF& rect);
    void onSwitchTrackerRequest(const quint8& code);
    void onNewFrame(const QSharedPointer< cv::Mat >& frame);

private:
    class Impl;
    Impl* d;
};

#endif //TRACKER_TASK_H
