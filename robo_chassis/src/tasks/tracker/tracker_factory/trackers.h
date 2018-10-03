#ifndef TRACKERS_H
#define TRACKERS_H

namespace va
{
    enum class TrackerCode
    {
        Unknown = 0,
        Kcf = 1,
        MedianFlow = 2,
        Boosting = 3,
        Mil = 4,
        Tld = 5,
        CustomTld = 6,
        OpenTld = 7
    };
}

#endif //TRACKERS_H
