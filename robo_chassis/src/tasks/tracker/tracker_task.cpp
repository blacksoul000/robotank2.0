#include "tracker_task.h"
#include "tracker_factory.h"
#include "trackers.h"

#include "pub_sub.h"

#include <QRectF>
#include <QPointF>
#include <QElapsedTimer>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/video/tracking.hpp"

namespace
{
    const double width = 240;
    const double height = 192;
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
    int imageWidth = 0;
    int imageHeight = 0;
    int64_t prevTime = 0;
    cv::Mat frame;
    QElapsedTimer fpsTimer;

    void onNewFrame(const cv::Mat& image);

    void publishTarget(const cv::Rect& rect);
    void publishDeviation(const cv::Rect& rect);
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

void TrackerTask::execute()
{
    d->fpsTimer.start();
    d->imageWidth = d->frame.cols;
    d->imageHeight = d->frame.rows;
    if (!d->tracker || !d->tracker->isTracking()) return;
    cv::Mat scaled;
    resize(d->frame, scaled, cv::Size(::width, ::height));
    d->tracker->track(scaled);

    d->publishTarget(d->tracker->target());
    d->publishDeviation(d->tracker->target());
    qDebug() << Q_FUNC_INFO << "Time: " << d->fpsTimer.elapsed();
}

void TrackerTask::onNewFrame(const cv::Mat& frame)
{
    d->frame = frame.clone();
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

void TrackerTask::Impl::publishTarget(const cv::Rect& rect)
{
    const double scaleX = ::width / imageWidth;
    const double scaleY = ::height / imageHeight;

    QRectF r(rect.x / scaleX, rect.y / scaleY, rect.width / scaleX, rect.height / scaleY);
    targetP->publish(r);
}

void TrackerTask::Impl::publishDeviation(const cv::Rect& rect)
{
    const double scaleX = ::width / imageWidth;
    const double scaleY = ::height / imageHeight;
    const double targetCenterX = (rect.x + rect.width) / 2;
    const double targetCenterY = (rect.y + rect.height) / 2;
    const double imageCenterX = imageWidth / 2;
    const double imageCenterY = imageHeight / 2;

    deviationP->publish(QPointF(imageCenterX - (targetCenterX / scaleX),
                                imageCenterY - (targetCenterY / scaleY)));
}
