#ifndef TRACKER_CAMSHIFT_H
#define TRACKER_CAMSHIFT_H

#include "itracker.h"

namespace va
{
    class TrackerCamshift : public ITracker
    {
    public:
        TrackerCamshift();
        ~TrackerCamshift() override;

        void start(const cv::Rect& rect) override;
        void stop() override;
        bool isTracking() const override;
        void track(const cv::Mat& image) override;
        cv::Rect target() const override;

    private:
        class Impl;
        Impl* d;
    };
}

#endif // TRACKER_CAMSHIFT_H
