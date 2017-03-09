#ifndef TLD_ADAPTER_H
#define TLD_ADAPTER_H

#include "itracker.h"

namespace va
{
    class TldAdapter : public ITracker
    {
    public:
        TldAdapter();
        ~TldAdapter() override;
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

#endif //TLD_ADAPTER_H
