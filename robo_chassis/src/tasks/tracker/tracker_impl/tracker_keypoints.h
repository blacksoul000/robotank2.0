#ifndef TRACKER_KEYPOINTS_H
#define TRACKER_KEYPOINTS_H

#include "itracker.h"

namespace va
{
    class TrackerKeypoints : public ITracker
    {
    public:
        TrackerKeypoints(const std::string& algo);
        ~TrackerKeypoints() override;

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

#endif //TRACKER_KEYPOINTS_H
