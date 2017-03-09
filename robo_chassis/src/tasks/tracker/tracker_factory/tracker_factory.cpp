#include "tracker_factory.h"

#include "tracker_camshift.h"
#include "tracker_keypoints.h"
#include "tld_adapter.h"
#include "opentld_adapter.h"

using namespace va;

ITracker* TrackerFactory::makeTracker(TrackerCode code)
{
    switch (code)
    {
        case TrackerCode::CamShift: return new va::TrackerCamshift();
        case TrackerCode::Boosting: return new va::TrackerKeypoints("BOOSTING");
        case TrackerCode::MedianFlow: return new va::TrackerKeypoints("MEDIANFLOW");
        case TrackerCode::Mil: return new va::TrackerKeypoints("MIL");
        case TrackerCode::Tld: return new va::TrackerKeypoints("TLD");
        case TrackerCode::CustomTld: return new va::TldAdapter();
        case TrackerCode::OpenTld: return new va::OpenTldAdapter();
        case TrackerCode::Unknown:
        default: break;
    }
    return new va::TrackerCamshift();
}
