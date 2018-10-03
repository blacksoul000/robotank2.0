#ifndef OPENTLD_ADAPTER_H
#define OPENTLD_ADAPTER_H

#include "itracker.h"

namespace va
{
    class OpenTldAdapter : public ITracker
    {
    public:
        OpenTldAdapter();
        ~OpenTldAdapter() override;
        void start(const cv::Rect2d& rect) override;
        void stop() override;
        bool isTracking() const override;
        void track(const cv::Mat& image) override;
        cv::Rect2d target() const override;

    private:
        class Impl;
        Impl* d;
    };
}  // namespace tracker

#endif //OPENTLD_ADAPTER_H
