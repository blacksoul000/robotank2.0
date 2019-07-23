#include "tracker_task.h"
#include "tracker_factory.h"
#include "trackers.h"

#include "pub_sub.h"

#include <QRectF>
#include <QPointF>
#include <QElapsedTimer>
#include <QThread>
#include <QDebug>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/video/tracking.hpp"

namespace
{
//    const double width = 320;
//    const double height = 240;
    const double width = 240;
    const double height = 180;

    const va::TrackerCode defaultTracker = va::TrackerCode::OpenTld;
}  // namespace

class TrackerTask::Impl
{
public:
    Publisher< QRectF >* targetP = nullptr;
    Publisher< QPointF >* deviationP = nullptr;
    Publisher< bool >* trackerStatusP = nullptr;

    va::ITracker* tracker = nullptr;
    va::TrackerCode trackAlgo =::defaultTracker;
    float imageWidth = 0.0;
    float imageHeight = 0.0;
    int64_t prevTime = 0;
    QElapsedTimer fpsTimer;
    QPointF gunPosition;

    void onNewFrame(const cv::Mat& image);

    void publishTarget(const cv::Rect2d& rect);
    void publishDeviation(const cv::Rect2d& rect);
};

TrackerTask::TrackerTask() :
    ITask(),
    d(new Impl)
{
    d->targetP = PubSub::instance()->advertise< QRectF >("tracker/target");
    d->deviationP = PubSub::instance()->advertise< QPointF >("tracker/deviation");
    d->trackerStatusP = PubSub::instance()->advertise< bool >("tracker/status");

    PubSub::instance()->subscribe("camera/image", &TrackerTask::onNewFrame, this);
    PubSub::instance()->subscribe("tracker/toggle", &TrackerTask::onToggleRequest, this);
    PubSub::instance()->subscribe("tracker/selector", &TrackerTask::onSwitchTrackerRequest, this);
    PubSub::instance()->subscribe("gun/position", &TrackerTask::onGunPosition, this);
    PubSub::instance()->subscribe("connection", &TrackerTask::onConnectionChanged, this);

    d->targetP->publish(QRectF());
    d->trackerStatusP->publish(false);
}

TrackerTask::~TrackerTask()
{
    delete d->targetP;
    delete d->deviationP;
    delete d->trackerStatusP;

    delete d->tracker;
    delete d;
}

void TrackerTask::start()
{
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
}

void TrackerTask::execute()
{}

void TrackerTask::onNewFrame(const QSharedPointer< cv::Mat >& frame)
{
    if (frame.isNull()) return;
//    d->fpsTimer.start();

    d->imageWidth = frame->cols;
    d->imageHeight = frame->rows;

    if (!d->tracker || !d->tracker->isTracking()) return;

    cv::Mat scaled;
    resize(*frame, scaled, cv::Size(::width, ::height));
    d->tracker->track(scaled);

    auto rect = d->tracker->target();

    const double scaleX = ::width / d->imageWidth;
    const double scaleY = ::height / d->imageHeight;

    cv::Rect2d r(rect.x / scaleX, rect.y / scaleY, rect.width / scaleX, rect.height / scaleY);

    d->publishTarget(d->tracker->target());
    d->publishDeviation(d->tracker->target());
}

void TrackerTask::onToggleRequest(const QRectF& rect)
{
    const bool start = !rect.isEmpty();
    if (d->tracker && d->tracker->isTracking() == start) return;

    if (start)
    {
        const double scaleX = ::width / d->imageWidth;
        const double scaleY = ::height / d->imageHeight;
        cv::Rect cvRect(rect.x() * scaleX, rect.y() * scaleY,
                        rect.width() * scaleX, rect.height() * scaleY);
        d->tracker = va::TrackerFactory::makeTracker(d->trackAlgo);
        d->tracker->start(cvRect);
    }
    else if(d->tracker)
    {
        d->tracker->stop();
        delete d->tracker;
        d->tracker = nullptr;

        d->targetP->publish(QRect());
    }

    d->trackerStatusP->publish(d->tracker && d->tracker->isTracking());
}

void TrackerTask::onSwitchTrackerRequest(const quint8& code)
{
    d->trackAlgo = va::TrackerCode(code);

    if (d->tracker)
    {
        cv::Rect cvRect = d->tracker->target();
        delete d->tracker;
        d->tracker = va::TrackerFactory::makeTracker(d->trackAlgo);
        d->tracker->start(cvRect);
    }
}

void TrackerTask::onGunPosition(const QPointF& position)
{
    d->gunPosition = position;
}

void TrackerTask::onConnectionChanged(const bool& online)
{
    if (!online) this->onToggleRequest(QRectF());
}

void TrackerTask::Impl::publishTarget(const cv::Rect2d& rect)
{
    const double scaleX = ::width / imageWidth;
    const double scaleY = ::height / imageHeight;

    QRectF r(rect.x / scaleX, rect.y / scaleY, rect.width / scaleX, rect.height / scaleY);
    targetP->publish(r);
}

void TrackerTask::Impl::publishDeviation(const cv::Rect2d& rect)
{
    if (rect.width == 0 || rect.height == 0) 
    {
        deviationP->publish(QPointF());
        return;
    }

    const double scaleX = ::width / imageWidth;
    const double scaleY = ::height / imageHeight;
    const double targetCenterX = rect.x + rect.width / 2;
    const double targetCenterY = rect.y + rect.height / 2;
    const double imageCenterX = imageWidth / 2;
    const double imageCenterY = imageHeight / 2;
    deviationP->publish(QPointF((targetCenterX / scaleX) - imageCenterX,
                                (targetCenterY / scaleY - imageCenterY)));
}
