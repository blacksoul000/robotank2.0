#include "tld_adapter.h"

#include "custom_tld.h"

#include <iostream>

using va::TldAdapter;

class TldAdapter::Impl
{
public:
    BoundingBox target;
    bool tracking = false;
    TLD* tracker;

    bool inited = false;
    cv::Mat prevImage;
};

TldAdapter::TldAdapter() :
    va::ITracker(),
    d(new Impl)
{
}

TldAdapter::~TldAdapter()
{
    delete d->tracker;
    delete d;
}

void TldAdapter::start(const cv::Rect& rect)
{
    d->tracking = true;
    d->target = BoundingBox(rect);
}

void TldAdapter::stop()
{
    d->tracking = false;
}

bool TldAdapter::isTracking() const
{
    return d->tracking;
}

cv::Rect TldAdapter::target() const
{
    return d->target;
}

void TldAdapter::track(const cv::Mat& image)
{
    cv::Mat gray;
    cvtColor(image, gray, CV_BGR2GRAY);
    flip(gray, gray, 1);

    if (!d->inited)
    {
        std::cout << "INIT" << std::endl;
        d->tracker = new TLD();
        if (!d->tracker->init(gray, d->target)) return;

        d->inited = true;
        swap(d->prevImage, gray);
        return;
    }

    bool status;
    d->tracker->processFrame(d->prevImage, gray, d->target, status);
    swap(d->prevImage, gray);
}
