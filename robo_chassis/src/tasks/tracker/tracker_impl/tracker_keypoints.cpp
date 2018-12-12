#include "tracker_keypoints.h"

#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>

#include <iostream>
#include <chrono>

//KCF
//MIL
//BOOSTING
//MEDIANFLOW
//TLD

using va::TrackerKeypoints;
using namespace cv;

class TrackerKeypoints::Impl
{
public:
    Rect2d target;
    bool tracking = false;
    std::string algo;

    Ptr<Tracker> tracker;
    bool inited = false;
};

TrackerKeypoints::TrackerKeypoints(const std::string& algo) :
    va::ITracker(),
    d(new Impl)
{
    d->algo = algo;
}

TrackerKeypoints::~TrackerKeypoints()
{
    delete d;
}

void TrackerKeypoints::start(const cv::Rect2d& rect)
{
    d->tracking = true;
    d->target = cv::Rect2d(rect);
}

void TrackerKeypoints::stop()
{
    d->tracking = false;
}

bool TrackerKeypoints::isTracking() const
{
    return d->tracking;
}

cv::Rect2d TrackerKeypoints::target() const
{
    return d->target;
}

void TrackerKeypoints::track(const cv::Mat& image)
{
    if (!d->inited)
    {
        d->tracker = Tracker::create(d->algo);

        if (!d->tracker->init(image, d->target)) return;
        d->inited = true;
    }
    d->tracker->update(image, d->target);
}
