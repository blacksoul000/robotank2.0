#include "video_source.h"
#include "rtsp_server.h"

//msgs
#include "image_settings.h"

#include "pub_sub.h"

#include <QPointF>
#include <QSize>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"

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

    int& argc;
    char** argv;

//    cv::VideoWriter writer;
    rtsp_server::RtspServer* rtsp = nullptr;

    Publisher< cv::Mat >* imageP = nullptr;
    Publisher< QPointF >* dotsPerDegreeP = nullptr;

    void initRtspServer();
    bool openCamera();
    void setQuality(int quality);
    void setBrightness(int brightness);
    void setContrast(int contrast);
    bool writeImage(const cv::Mat& mat);

    Impl(int& argc, char** argv) : argc(argc), argv(argv) {}
};

VideoSource::VideoSource(int& argc, char** argv) :
    d(new Impl(argc, argv))
{
    d->imageP = PubSub::instance()->advertise< cv::Mat >("camera/image");
    d->dotsPerDegreeP = PubSub::instance()->advertise< QPointF >("camera/dotsPerDegree");

    PubSub::instance()->subscribe("camera/settings", &VideoSource::onImageSettingsChanged, this);
}

VideoSource::~VideoSource()
{
    d->capturer.release();
    delete d->rtsp;
//    d->writer.release();
    delete d;
}

void VideoSource::start()
{
    d->initRtspServer();
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
    if (!d->writeImage(frame))
    {
        // TODO - stop video source
        d->rtsp->stop();
    }
//    d->writer << frame;
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

void VideoSource::Impl::initRtspServer()
{
    qDebug() << Q_FUNC_INFO ;
    rtsp = new rtsp_server::RtspServer(argc, argv,
                                       "videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 sync=true",
//                                       "videoconvert ! x264enc noise-reduction=10000 "
//                                       "tune=zerolatency byte-stream=true threads=1 !"
//                                       "rtph264pay name=pay0 pt=96",
                                       ::width, ::height, 25, "GRAY8");
    rtsp->setHost("127.0.0.1");
    rtsp->setPort(8554);
    rtsp->setStreamName("live");

    rtsp->start();
}

bool VideoSource::Impl::openCamera()
{
    qDebug() << Q_FUNC_INFO ;
#ifdef PICAM
    // set camera params
    // 2592x1944 - native resolution
    capturer.set(CV_CAP_PROP_FORMAT, CV_8UC3);
    capturer.set(CV_CAP_PROP_FRAME_HEIGHT, ::height);
    capturer.set(CV_CAP_PROP_FRAME_WIDTH, ::width);

    if(!capturer.open())
#else
    capturer.set(CV_CAP_PROP_FORMAT, CV_8UC3);
    capturer.set(CV_CAP_PROP_FRAME_WIDTH, ::width);
    capturer.set(CV_CAP_PROP_FRAME_HEIGHT, ::height);

//    if(!capturer.open(0))
    if(!capturer.open("/tmp/test.avi"))
#endif //PICAM
    {
        qDebug() << Q_FUNC_INFO << "Failed to open camera";
        return false;
    }

////    libv4l-dev
//    writer.open("appsrc ! udpsink host=localhost port=5000", 0, 25, cv::Size(640, 480), true);
//    writer.open("appsrc ! videoconvert ! x264enc noise-reduction=10000 tune=zerolatency byte-stream=true threads=4 ! rstpclientsink  name=sink location=rtsp://127.0.0.1:8554/live", 0, 25, cv::Size(width, height), true);
//    if (!writer.isOpened()) {
//        qDebug() << "Can't create video writer";
//        return false;
//    }
//---------------------------
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

bool VideoSource::Impl::writeImage(const cv::Mat& mat)
{
    const bool color = false;
    cv::Mat result;

    if (color)
    {
        // TODO - convert to RGB16
    }
    else
    {
        cv::cvtColor(mat, result, CV_BGR2GRAY);
    }
    constexpr int size = ::width * ::height * (color ? 2 : 1);
    return rtsp->write((char*)result.data, size);
}
