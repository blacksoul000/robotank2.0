#include "opentld_adapter.h"

#include "libopentld/tld/TLD.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.h>
#include "opencv2/core/utility.hpp"

#include <iostream>

using va::OpenTldAdapter;

class OpenTldAdapter::Impl
{
public:
    cv::Rect target;
    bool tracking = false;
    tld::TLD* tracker;

    bool inited = false;
};

OpenTldAdapter::OpenTldAdapter() :
    va::ITracker(),
    d(new Impl)
{
}

OpenTldAdapter::~OpenTldAdapter()
{
    delete d->tracker;
    delete d;
}

void OpenTldAdapter::start(const cv::Rect& rect)
{
    d->tracking = true;
    d->target = rect;
}

void OpenTldAdapter::stop()
{
    d->tracking = false;
}

bool OpenTldAdapter::isTracking() const
{
    return d->tracking;
}

cv::Rect OpenTldAdapter::target() const
{
    return d->target;
}

void OpenTldAdapter::track(const cv::Mat& image)
{
    if (!d->inited)
    {
        std::cout <<  std::endl << "INIT" << std::endl;
        cv::Mat gray;
        cvtColor(image, gray, CV_BGR2GRAY);
        d->tracker = new tld::TLD();
        d->tracker->learningEnabled = false;

        d->tracker->detectorCascade->imgWidth = gray.cols;
        d->tracker->detectorCascade->imgHeight = gray.rows;
        d->tracker->detectorCascade->imgWidthStep = gray.step;
        d->tracker->detectorCascade->useShift = true;
        d->tracker->detectorCascade->shift = 0.1;
        d->tracker->detectorCascade->minScale = -10;
        d->tracker->detectorCascade->maxScale = 10;
        d->tracker->detectorCascade->minSize = 25;
        d->tracker->detectorCascade->numTrees = 10;
        d->tracker->detectorCascade->numFeatures = 13;
        d->tracker->detectorCascade->nnClassifier->thetaTP = 0.65;
        d->tracker->detectorCascade->nnClassifier->thetaFP = 0.5;

        d->tracker->selectObject(gray, &d->target);
        d->inited = true;
        d->tracker->learningEnabled = true;
    }

    d->tracker->processImage(image);
    d->target = d->tracker->currBB ? *d->tracker->currBB : cv::Rect();
}
