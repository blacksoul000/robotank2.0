#ifndef TRACKER_TASK_H
#define TRACKER_TASK_H

#include "i_task.h"

#include <QSharedPointer>

namespace cv
{
    class Mat;
}

class QPointF;
class TrackerTask : public ITask
{
public:
    TrackerTask();
    ~TrackerTask();

    void start() override;
    void execute() override;

    void onDotsPerDegreeChanged(const QPointF& dpd);
    void onToggleRequest(const QRectF& rect);
    void onSwitchTrackerRequest(const quint8& code);
    void onNewFrame(const QSharedPointer< cv::Mat >& frame);
    void onGunPosition(const QPointF& position);
    void onConnectionChanged(const bool& online);

private:
    class Impl;
    Impl* d;
};

#endif //TRACKER_TASK_H
