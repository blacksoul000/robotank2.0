#include "empty.h"
#include "joy_axes.h"
#include "influence.h"
#include "point3d.h"
#include "image_settings.h"

#include <QDebug>

#include "opencv2/core/core.hpp"

inline QDebug operator<< (QDebug d, const Empty& e) {
    return d;
}
inline QDebug operator<< (QDebug d, const JoyAxes& e) {
    d << e.axes;
    return d;
}
inline QDebug operator<< (QDebug d, const Influence& e) {
    return d;
}
inline QDebug operator<< (QDebug d, const Point3D& e) {
    d << e.x << e.y << e.z;
    return d;
}
inline QDebug operator<< (QDebug d, const cv::Mat& e) {
    return d;
}
inline QDebug operator<< (QDebug d, const ImageSettings& e) {
    d << e.quality << e.brightness << e.contrast;
    return d;
}