#include "tracker_keypoints.h"

#ifdef WITH_OPENCV_TRACKING
	#include <opencv2/core/utility.hpp>
	#include <opencv2/tracking.hpp>
#endif // WITH_OPENCV_TRACKING

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

#ifdef WITH_OPENCV_TRACKING
    Ptr<Tracker> tracker;
    Ptr<Tracker> createTracker();
#endif // WITH_OPENCV_TRACKING

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
#ifdef WITH_OPENCV_TRACKING
    if (!d->inited)
    {
        d->tracker = d->createTracker();
        if (!d->tracker) return;

        if (!d->tracker->init(image, d->target)) return;
        d->inited = true;
    }
    d->tracker->update(image, d->target);
#endif // WITH_OPENCV_TRACKING
}

#ifdef WITH_OPENCV_TRACKING
Ptr<Tracker> TrackerKeypoints::Impl::createTracker()
{
    #if (CV_MINOR_VERSION < 3)
    {
        return Tracker::create(algo);
    }
    #else
    {
        if (algo == "BOOSTING") return TrackerBoosting::create();
        if (algo == "MIL") return TrackerMIL::create();
        if (algo == "KCF") return TrackerKCF::create();
        if (algo == "TLD") return TrackerTLD::create();
        if (algo == "MEDIANFLOW") return TrackerMedianFlow::create();
        if (algo == "GOTURN") return TrackerGOTURN::create();
        if (algo == "MOSSE") return TrackerMOSSE::create();
        if (algo == "CSRT") return TrackerCSRT::create();
    }
    #endif
    return Ptr<Tracker>();
}
#endif // WITH_OPENCV_TRACKING