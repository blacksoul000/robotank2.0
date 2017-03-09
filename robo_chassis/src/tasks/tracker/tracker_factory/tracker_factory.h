#ifndef TRACKER_FACTORY_H
#define TRACKER_FACTORY_H

#include "trackers.h"
#include "itracker.h"

namespace va
{
    class TrackerFactory
    {
    public:
        static ITracker* makeTracker(va::TrackerCode code);

    private:
        TrackerFactory(){}
    };
}

#endif //TRACKER_FACTORY_H
