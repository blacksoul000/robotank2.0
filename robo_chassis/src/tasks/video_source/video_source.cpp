#include "video_source.h"
#include "rtsp_server.h"

//msgs
#include "image_settings.h"

#include "pub_sub.h"

#include <QHostAddress>
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

    rtsp_server::RtspServer* rtsp = nullptr;

    Publisher< cv::Mat >* imageP = nullptr;
    Publisher< QPointF >* dotsPerDegreeP = nullptr;

    QHostAddress streamHost;
    quint16 streamPort;
    QString streamName;

    void initRtspServer();
    bool openCamera();
    void setQuality(int quality);
    void setBrightness(int brightness);
    void setContrast(int contrast);
    bool writeImage(const cv::Mat& mat);
    bool parseVideoSource(const QString& source);

    Impl(int& argc, char** argv) : argc(argc), argv(argv) {}
};

VideoSource::VideoSource(int& argc, char** argv) :
    d(new Impl(argc, argv))
{
    d->imageP = PubSub::instance()->advertise< cv::Mat >("camera/image");
    d->dotsPerDegreeP = PubSub::instance()->advertise< QPointF >("camera/dotsPerDegree");

    PubSub::instance()->subscribe("camera/settings", &VideoSource::onImageSettingsChanged, this);
    PubSub::instance()->subscribe("camera/source", &VideoSource::onVideoSourceChanged, this);
}

VideoSource::~VideoSource()
{
    d->capturer.release();
    delete d->imageP;
    delete d->dotsPerDegreeP;
    delete d->rtsp;
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
    if (!d->writeImage(frame))
    {
        // TODO - stop video source
        d->rtsp->stop();
    }
}

void VideoSource::onImageSettingsChanged(const ImageSettings& settings)
{
    qDebug() << Q_FUNC_INFO << settings.quality << settings.brightness << settings.contrast;
    d->setBrightness(settings.brightness);
    d->setContrast(settings.contrast);
}

void VideoSource::onVideoSourceChanged(const QString& source)
{
    if (d->rtsp) delete d->rtsp;

    if (!d->parseVideoSource(source))
    {
        qWarning() << "Invalid video source requested. "
            "Try something like 'rtsp://127.0.0.1:8554/live'";
        return;
    }

    d->initRtspServer();
}

//----------------------------------------------------------------------------
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
    rtsp = new rtsp_server::RtspServer(argc, argv,
                                       "videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 sync=true",
//                                       "videoconvert ! x264enc noise-reduction=10000 "
//                                       "tune=zerolatency byte-stream=true threads=1 !"
//                                       "rtph264pay name=pay0 pt=96 sync=true",
                                       ::width, ::height, 25, "BGR");
    rtsp->setHost(streamHost.toString().toStdString());
    rtsp->setPort(streamPort);
    rtsp->setStreamName(streamName.toStdString());

    rtsp->start();
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
    capturer.set(CV_CAP_PROP_FORMAT, CV_8UC3);
    capturer.set(CV_CAP_PROP_FRAME_WIDTH, ::width);
    capturer.set(CV_CAP_PROP_FRAME_HEIGHT, ::height);

    if(!capturer.open(0))
#endif //PICAM
    {
        qDebug() << Q_FUNC_INFO << "Failed to open camera";
        return false;
    }

    this->dotsPerDegreeP->publish(QPointF(::width / ::fieldOfViewH, ::height / ::fieldOfViewV));
    return true;
}

bool VideoSource::Impl::writeImage(const cv::Mat& mat)
{
    const int size = ::width * ::height * mat.channels();
    return rtsp->write((char*)mat.data, size);
}

bool VideoSource::Impl::parseVideoSource(const QString& source)
{
    if (!source.startsWith("rtsp://")) return false;
    QStringList splitted = source.split("/");
    if (splitted.count() != 4) return false;

    QStringList address = splitted.at(2).split(":"); // host:port
    if (address.count() != 2) return false;
    streamHost = QHostAddress(address.at(0));
    if (streamHost.isNull()) return false;
    bool ok;
    streamPort = address.at(1).toInt(&ok);
    if (!ok) return false;
    streamName = splitted.last();
    return true;
}
