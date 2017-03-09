#include "tracker_camshift.h"

#include "opencv2/video/tracking.hpp"

#include <iostream>

using va::TrackerCamshift;
using namespace cv;

namespace
{
    const int vmin = 10;
    const int vmax = 256;
    const int smin = 30;
    const int hsize = 16;
}

class TrackerCamshift::Impl
{
public:
    Rect target;
    Rect trackWindow;
    bool tracking = false;
    int trackObject = -1;
    float hranges[2] = {0, 180};
    const float* phranges = hranges;

    Mat frame, hsv, hue, mask, hist = Mat::zeros(200, 320, CV_8UC3), backproj;
};

TrackerCamshift::TrackerCamshift() :
    va::ITracker(),
    d(new Impl)
{
}

TrackerCamshift::~TrackerCamshift()
{
    delete d;
}

void TrackerCamshift::start(const cv::Rect& rect)
{
    d->tracking = true;
    d->target = rect;
}

void TrackerCamshift::stop()
{
    d->tracking = false;
}

bool TrackerCamshift::isTracking() const
{
    return d->tracking;
}

cv::Rect TrackerCamshift::target() const
{
    return d->trackWindow;
}

void TrackerCamshift::track(const cv::Mat& image)
{
    cvtColor(image, d->hsv, COLOR_BGR2HSV);

    inRange(d->hsv, Scalar(0, ::smin, MIN(::vmin, ::vmax)),
            Scalar(180, 256, MAX(::vmin, ::vmax)), d->mask);
    int ch[] = {0, 0};
    d->hue.create(d->hsv.size(), d->hsv.depth());
    mixChannels(&d->hsv, 1, &d->hue, 1, ch, 1);

    if(d->trackObject < 0)
    {
        Mat roi(d->hue, d->target);
        Mat maskroi(d->mask, d->target);
        calcHist(&roi, 1, 0, maskroi, d->hist, 1, &::hsize, &d->phranges);
        normalize(d->hist, d->hist, 0, 255, NORM_MINMAX);

        d->trackWindow = d->target;
        d->trackObject = 1;
    }

    calcBackProject(&d->hue, 1, 0, d->hist, d->backproj, &d->phranges);
    d->backproj &= d->mask;
    CamShift(d->backproj, d->trackWindow,
             TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
    if( d->trackWindow.area() <= 1 )
    {
        int cols = d->backproj.cols, rows = d->backproj.rows, r = (MIN(cols, rows) + 5)/6;
        d->trackWindow = Rect(d->trackWindow.x - r, d->trackWindow.y - r,
                           d->trackWindow.x + r, d->trackWindow.y + r) &
                      Rect(0, 0, cols, rows);
    }
//    rectangle( image, d->trackWindow, Scalar(0,0,255));

//    namedWindow( "CamShift Demo", WINDOW_AUTOSIZE );    // Create a window for display.
//    imshow( "CamShift Demo", image );                   // Show our image inside it.
//    waitKey(10);
}
