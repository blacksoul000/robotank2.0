#ifndef TRACKERS_H
#define TRACKERS_H

namespace va
{
    enum class TrackerCode
    {
        Unknown,
        CamShift,
        MedianFlow,
        Boosting,
        Mil,
        Tld,
        CustomTld,
        OpenTld
    };
}

#endif //TRACKERS_H
