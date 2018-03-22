#include "video_source.h"
#include "rtsp_server.h"

//msgs
#include "image_settings.h"

#include "pub_sub.h"

#include <QPointF>
#include <QSize>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QSharedPointer>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"

using MatPtr = QSharedPointer< cv::Mat >;

namespace
{
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
    int& argc;
    char** argv;

    rtsp_server::RtspServer* rtsp = nullptr;
    bool started = false;

    Publisher< MatPtr >* imageP = nullptr;
    Publisher< QPointF >* dotsPerDegreeP = nullptr;
    Publisher< QString >* videoSourceP = nullptr;

    void initRtspServer();
    void setBrightness(int brightness);
    void setContrast(int contrast);
    QString localIp() const;

    Impl(int& argc, char** argv) : argc(argc), argv(argv) {}
};

VideoSource::VideoSource(int& argc, char** argv) :
    d(new Impl(argc, argv))
{
    d->imageP = PubSub::instance()->advertise< MatPtr >("camera/image");
    d->dotsPerDegreeP = PubSub::instance()->advertise< QPointF >("camera/dotsPerDegree");
    d->videoSourceP = PubSub::instance()->advertise< QString >("camera/source");

    PubSub::instance()->subscribe("camera/settings", &VideoSource::onImageSettingsChanged, this);
}

VideoSource::~VideoSource()
{
    delete d->imageP;
    delete d->dotsPerDegreeP;
    delete d->videoSourceP;
    delete d->rtsp;
    delete d;
}

void VideoSource::start()
{
    if (d->localIp().isEmpty()) return;

    d->initRtspServer();
    QString path = QString("rtsp://%1:%2/%3")
        .arg(d->localIp())
        .arg(d->rtsp->port())
        .arg(QString::fromStdString(d->rtsp->streamName()));
    d->videoSourceP->publish(path);
    d->dotsPerDegreeP->publish(QPointF(::width / ::fieldOfViewH, ::height / ::fieldOfViewV));
    d->started = true;
}

void VideoSource::execute()
{
    if (!d->started) 
    {
        this->start();
        return;
    }

    d->rtsp->spin();
    auto data = d->rtsp->lastFrame();
    if (!data) return;

    MatPtr mat = MatPtr::create(cv::Size(::width, ::height), CV_8UC3);
    memcpy(mat->data, data, mat->total() * mat->elemSize());
    d->imageP->publish(mat);
}

void VideoSource::onImageSettingsChanged(const ImageSettings& settings)
{
    d->setBrightness(settings.brightness);
    d->setContrast(settings.contrast);
}

//----------------------------------------------------------------------------
void VideoSource::Impl::setBrightness(int brightness)
{
//    capturer.set(CV_CAP_PROP_BRIGHTNESS, brightness);
}

void VideoSource::Impl::setContrast(int contrast)
{
//    capturer.set(CV_CAP_PROP_CONTRAST, contrast);
}

void VideoSource::Impl::initRtspServer()
{
//    rpicamsrc sensor-mode=5 bitrate=2097152 do-timestamp=true ! video/x-h264,width=1024,height=768,framerate=30/1
//             ! tee name=t t. ! queue ! rtph264pay name=pay0 pt=96 t. ! queue ! h264parse
//             ! omxh264dec ! videoconvert ! fakesink name=internal

    rtsp = new rtsp_server::RtspServer(argc, argv,
               QString("rpicamsrc sensor-mode=5 bitrate=%1 do-timestamp=true ! "
                       "video/x-h264,width=%2,height=%3,framerate=%4/1 ! "
                       "tee name=t t. ! queue ! rtph264pay name=pay0 pt=96 t. ! queue ! "
                       "h264parse ! omxh264dec ! videoconvert ! videorate ! "
                       "video/x-raw,framerate=10/1,format=BGR ! fakesink name=internal ")

//               QString("rpicamsrc sensor-mode=5 bitrate=%1 do-timestamp=true ! "
//                       "video/x-h264,width=%2,height=%3,framerate=%4/1 ! "
//                       "rtph264pay name=pay0 pt=96 ! h264parse ! omxh264dec ! "
//                       "videoconvert ! fakesink name=internal ")
                                       .arg(2097152)
                                       .arg(::width)
                                       .arg(::height)
                                       .arg(25)
                                       .toStdString());

    rtsp->start();
}

QString VideoSource::Impl::localIp() const
{
    for (const QHostAddress& address: QNetworkInterface::allAddresses()) 
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol 
            && address != QHostAddress(QHostAddress::LocalHost))
        {
             return address.toString();
        }
    }
    return QString();
}
