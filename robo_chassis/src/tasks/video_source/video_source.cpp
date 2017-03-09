#include "video_source.h"

//msgs
#include "image_settings.h"

#include "pub_sub.h"

#include <QPointF>

#include <opencv2/highgui/highgui.hpp>

#ifdef PICAM
#include <raspicam/raspicam_cv.h>
#endif

namespace
{
    const int defaultQuality = 15;
    const int defaultBrightness = 65;
    const int defaultContrast = 75;
    const int width = 640;
    const int height = 480;

#ifdef PICAM
    const double fieldOfViewH = 53.5; // +/- 0.13 degrees
    const double fieldOfViewV = 41.41; // +/- 0.11 degress
#else
    // set proper values for used camera
    const double fieldOfViewH = 53.5; // +/- 0.13 degrees
    const double fieldOfViewV = 41.41; // +/- 0.11 degress
#endif // PICAM
} //namespace

class VideoSource::Impl
{
public:
#ifdef PICAM
    raspicam::RaspiCam_Cv capturer;
#else
    cv::VideoCapture capturer;
#endif //PICAM

    cv::VideoWriter writer;

    Publisher< cv::Mat >* imageP = nullptr;
    Publisher< QPointF >* dotsPerDegreeP = nullptr;
    bool openCamera();

    void setQuality(int quality);
    void setBrightness(int brightness);
    void setContrast(int contrast);
};

VideoSource::VideoSource() :
    d(new Impl)
{
    d->imageP = PubSub::instance()->advertise< cv::Mat >("camera/image");
    d->dotsPerDegreeP = PubSub::instance()->advertise< QPointF >("camera/dotsPerDegree");

    PubSub::instance()->subscribe("camera/settings", &VideoSource::onImageSettingsChanged, this);
}

VideoSource::~VideoSource()
{
    d->capturer.release();
    d->writer.release();
    delete d;
}

void VideoSource::start()
{
    d->openCamera();
}

void VideoSource::execute()
{
    if (!d->capturer.isOpened()) return;

    cv::Mat frame;
#ifdef PICAM
    d->capturer.grab();
    d->capturer.retrieve(frame);
#else
     d->capturer >> frame;
#endif //PICAM

    if(frame.empty()) return;

    d->imageP->publish(frame);
    d->writer << frame;
}

void VideoSource::onImageSettingsChanged(const ImageSettings& settings)
{
    qDebug() << Q_FUNC_INFO << settings.quality << settings.brightness << settings.contrast;
    d->setQuality(settings.quality);
    d->setBrightness(settings.brightness);
    d->setContrast(settings.contrast);
}

//----------------------------------------------------------------------------
void VideoSource::Impl::setQuality(int quality)
{
////    rosrun dynamic_reconfigure dynparam set -t1 /camera/image/compressed jpeg_quality 15
//    std::string s = "rosrun dynamic_reconfigure dynparam set -t1 "
//                    "/camera/image/compressed jpeg_quality "
//            + std::to_string(quality);
//    popen(s.c_str(), "r");
////    FILE* f = popen(s.c_str(), "r");
////    pclose(f);
}

void VideoSource::Impl::setBrightness(int brightness)
{
    capturer.set(CV_CAP_PROP_BRIGHTNESS, brightness);
}

void VideoSource::Impl::setContrast(int contrast)
{
    capturer.set(CV_CAP_PROP_CONTRAST, contrast);
}

bool VideoSource::Impl::openCamera()
{
#ifdef PICAM
    // set camera params
    // 2592x1944 - native resolution
    capturer.set(CV_CAP_PROP_FORMAT, CV_8UC3);
    capturer.set(CV_CAP_PROP_FRAME_HEIGHT, ::height);
    capturer.set(CV_CAP_PROP_FRAME_WIDTH, ::width);

    if(!capturer.open())
#else
    capturer.set(CV_CAP_PROP_FRAME_WIDTH, ::width);
    capturer.set(CV_CAP_PROP_FRAME_HEIGHT, ::height);

    if(!capturer.open(0))
#endif //PICAM
    {
        qDebug() << Q_FUNC_INFO << "Failed to open camera";
        return false;
    }
//    libv4l-dev

    // http://answers.opencv.org/question/96178/gstreamer-output-with-videowriter/
//!!    writer.open("appsrc ! videoconvert ! filesink location=/tmp/video.mkv", 0, 25, cv::Size(width, height), true);
    writer.open("appsrc ! videoconvert ! x264enc noise-reduction=10000 tune=zerolatency byte-stream=true threads=4 ! udpsink host=localhost port=5000", 0, 25, cv::Size(width, height), true);
//    writer.open("appsrc ! videoconvert ! theoraenc bitrate=150 ! filesink location=/tmp/video.mkv", 0, 25, cv::Size(width, height), true);
//!!    writer.open("appsrc ! filesink location=/tmp/video.mkv sync=false", 0, 25, cv::Size(640, 480), true);
//    writer.open("appsrc ! video/x-raw-rgb,width=640,height=480 !filesink location=/tmp/video", 0, 30, cv::Size(640, 480), true);
//    writer.open("appsrc ! video/x-raw-rgb,width=640,height=480 ! ffmpegcolorspace ! videoscale method=1 ! theoraenc bitrate=150 ! tcpserversink host=127.0.0.1 port=5000", 0, 30, cv::Size(640, 480), true);
//--    writer.open("appsrc ! autovideoconvert ! v4l2video1h264enc extra-controls=\"encode,h264_level=10,h264_profile=4,frame_level_rate_control_enable=1,video_bitrate=2000000\" ! h264parse ! rtph264pay config-interval=1 pt=96 ! udpsink host=127.0.0.1 port=54000 sync=false", 0, 30, cv::Size(640, 480), true);
//--    writer.open("appsrc ! videoconvert ! x264enc noise-reduction=10000 tune=zerolatency byte-stream=true threads=4 ! mpegtsmux ! udpsink host=localhost port=9999", 0 , (double)30, cv::Size(640, 480), true);
//    writer.open("appsrc ! autovideosink", cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'), 30.0, cv::Size(640, 480), true);
//--    writer.open("appsrc ! videoconvert ! x264enc ! mpegtsmux ! udpsink host=localhost port=5000", 0, 25, cv::Size(width, height), true);
//    writer.open("appsrc ! identity silent=false ! x264enc ! mpegtsmux ! udpsink -v host=localhost port=5000", 0, 25, cv::Size(width, height), true);
    if (!writer.isOpened()) {
        qDebug() << "Can't create video writer";
        return false;
    }

//    TODO - from settings
//    int brightness = 0;
//    ros::param::param< int >("/camera/image/brightness", brightness, ::defaultBrightness);
//    d->setBrightness(brightness);

//    int contrast = 0;
//    ros::param::param< int >("/camera/image/contrast", contrast, ::defaultContrast);
//    d->setContrast(contrast);

//    int quality = 0;
//    ros::param::param< int >("/camera/image/quality", quality, ::defaultQuality;
//    d->setQuality(quality);


    this->dotsPerDegreeP->publish(QPointF(::width / ::fieldOfViewH, ::height / ::fieldOfViewV));
    return true;
}
