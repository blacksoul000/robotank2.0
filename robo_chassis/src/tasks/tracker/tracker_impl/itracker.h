#ifndef ITRACKER_H
#define ITRACKER_H

#include <opencv2/highgui/highgui.hpp>

namespace cv
{
    class Mat;
}

namespace va
{
    class ITracker
    {
    public:
        virtual ~ITracker() {}

        virtual void start(const cv::Rect2d& rect) = 0;
        virtual void stop() = 0;
        virtual bool isTracking() const = 0;
        virtual void track(const cv::Mat& image) = 0;
        virtual cv::Rect2d target() const = 0;
    };
}

#endif //ITRACKER_H
