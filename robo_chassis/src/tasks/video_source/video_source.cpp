#include "video_source.h"

#ifdef WITH_RTSP
	#include "rtsp_server.h"
#endif // WITH_RTSP

//msgs
#include "image_settings.h"

#include "pub_sub.h"

#include "abstract_link.h"

#include <QPointF>
#include <QSize>
#include <QSharedPointer>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDateTime>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"

using MatPtr = QSharedPointer< cv::Mat >;

namespace
{
    const int defaultBrightness = 65;
    const int defaultContrast = 75;
    const int width = 640;
    const int height = 480;

//    const int width = 1296;
//    const int height = 730;

#ifdef PICAM
    const double fieldOfViewH = 53.5; // +/- 0.13 degrees
//    const double fieldOfViewV = 41.41; // +/- 0.11 degress
    const double fieldOfViewV = 40; // +/- 0.11 degress
#else
    // set proper values for used camera
    const double fieldOfViewH = 53.5; // +/- 0.13 degrees
    const double fieldOfViewV = 41.41; // +/- 0.11 degress
#endif // PICAM
} //namespace

class VideoSource::Impl
{
public:
    bool started = false;

    Publisher< MatPtr >* imageP = nullptr;
    Publisher< QPointF >* dotsPerDegreeP = nullptr;
    Publisher< QString >* videoSourceP = nullptr;

#ifdef WITH_RTSP
    rtsp_server::RtspServer* rtsp = nullptr;
    void initRtspServer();
#endif // WITH_RTSP

    void setBrightness(int brightness);
    void setContrast(int contrast);
};

VideoSource::VideoSource() :
    d(new Impl)
{
    d->imageP = PubSub::instance()->advertise< MatPtr >("camera/image");
    d->dotsPerDegreeP = PubSub::instance()->advertise< QPointF >("camera/dotsPerDegree");
    d->videoSourceP = PubSub::instance()->advertise< QString >("camera/source");

    PubSub::instance()->subscribe("camera/settings", &VideoSource::onImageSettingsChanged, this);
    PubSub::instance()->subscribe("connection", &VideoSource::onConnectionChanged, this);
}

VideoSource::~VideoSource()
{
    delete d->imageP;
    delete d->dotsPerDegreeP;
    delete d->videoSourceP;

#ifdef WITH_RTSP
    delete d->rtsp;
#endif // WITH_RTSP

    delete d;
}

void VideoSource::start()
{
    d->dotsPerDegreeP->publish(QPointF(::width / ::fieldOfViewH, ::height / ::fieldOfViewV));

#ifdef WITH_RTSP
    d->initRtspServer();
    d->rtsp->setDataCallback(std::bind(&VideoSource::onNewFrame, this,
                                       std::placeholders::_1, std::placeholders::_2));
#endif // WITH_RTSP
}

void VideoSource::onNewFrame(const void* data, int size)
{
//    auto ts = QDateTime::currentDateTime().toString("hh.mm.ss.zzz");
    MatPtr mat = MatPtr::create(::height, ::width, CV_8UC1);
    memcpy(mat->data, data, ::height * ::width); // read only Y from YUV420.
//    cv::putText(*mat, ts.toStdString(), cvPoint(30,300), cv::FONT_HERSHEY_COMPLEX_SMALL, 4, cv::Scalar(100,200,250), 1, CV_AA);
    d->imageP->publish(mat);
}

void VideoSource::onImageSettingsChanged(const ImageSettings& settings)
{
    d->setBrightness(settings.brightness);
    d->setContrast(settings.contrast);
}

void VideoSource::onConnectionChanged(const data_source::AbstractLinkPtr& link)
{
#ifdef WITH_RTSP
    if (link)
    {
        if (d->started) return;

        for (const QHostAddress &address: QNetworkInterface::allAddresses())
        {
            if (address.isInSubnet(link->send().address(), 24))
            {
                QString path = QString("rtsp://%1:%2/%3")
                    .arg(address.toString())
                    .arg(d->rtsp->port())
                    .arg(QString::fromStdString(d->rtsp->streamName()));
                qDebug() << "Stream path:" << path;
                d->videoSourceP->publish(path);
                d->started = true;

                break;
            }
        }
    }
#endif // WITH_RTSP
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

#ifdef WITH_RTSP
void VideoSource::Impl::initRtspServer()
{
    rtsp = new rtsp_server::RtspServer(
               QString("rpicamsrc do-timestamp=true name=src ! "
                       "video/x-raw,width=%1,height=%2,framerate=%3/1,format=I420 ! "
//                       "omxh264enc ! video/x-h264,profile=baseline,quality=1,low-latency=true,key-int-max=1,speed-preset=ultrafast,control-rate=3 ! "
                       "omxh264enc ! video/x-h264,profile=baseline,speed-preset=ultrafast,tune=zerolatency ! "
                       "rtph264pay name=pay0 pt=96")
                                       .arg(::width)
                                       .arg(::height)
                                       .arg(25)
                                       .toStdString());
    rtsp->start();
}
#endif // WITH_RTSP
