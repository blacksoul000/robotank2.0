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
        void start(const cv::Rect& rect) override;
        void stop() override;
        bool isTracking() const override;
        void track(const cv::Mat& image) override;
        cv::Rect target() const override;

    private:
        class Impl;
        Impl* d;
    };
}  // namespace tracker

#endif //OPENTLD_ADAPTER_H
