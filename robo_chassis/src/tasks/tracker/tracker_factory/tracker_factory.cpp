#include "tracker_factory.h"

#include "tracker_keypoints.h"
#include "tld_adapter.h"
#include "opentld_adapter.h"

using namespace va;

ITracker* TrackerFactory::makeTracker(TrackerCode code)
{
    switch (code)
    {
        case TrackerCode::Kcf: return new va::TrackerKeypoints("KCF");
        case TrackerCode::Boosting: return new va::TrackerKeypoints("BOOSTING");
        case TrackerCode::MedianFlow: return new va::TrackerKeypoints("MEDIANFLOW");
        case TrackerCode::Mil: return new va::TrackerKeypoints("MIL");
        case TrackerCode::Tld: return new va::TrackerKeypoints("TLD");
        case TrackerCode::CustomTld: return new va::TldAdapter();
        case TrackerCode::OpenTld:
        case TrackerCode::Unknown:
        default: return new va::OpenTldAdapter();
    }
}
