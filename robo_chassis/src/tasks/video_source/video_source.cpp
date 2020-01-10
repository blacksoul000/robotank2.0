#include "video_source.h"

#include "video_server.h"

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
using std::placeholders::_1;
using std::placeholders::_2;

namespace
{
    const int defaultBrightness = 65;
    const int defaultContrast = 75;

    const int width = 1280;
    const int height = 720;
    const int fps = 25;

    const int videoPort = 5050;

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

// gst-launch-1.0 videotestsrc do-timestamp=true ! video/x-raw,width=800,height=600,framerate=25/1 ! 
// autovideoconvert ! jpegenc ! rtpjpegpay ! udpsink host=127.0.0.1 port=5000

// gst-launch-1.0 udpsrc port=5000 ! application/x-rtp,encoding-name=JPEG,payload=26 ! 
// rtpjpegdepay ! jpegdec ! autovideosink


class VideoSource::Impl
{
public:
    Publisher< MatPtr >* imageP = nullptr;
    Publisher< QPointF >* dotsPerDegreeP = nullptr;
    Publisher< QString >* videoSourceP = nullptr;

    VideoServer* video = nullptr;

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
    delete d;
}

void VideoSource::start()
{
    d->dotsPerDegreeP->publish(QPointF(::width / ::fieldOfViewH, ::height / ::fieldOfViewV));

    d->video = new VideoServer(::videoPort, ::width, ::height, ::fps, this);
    d->video->setDataCallback(std::bind(&VideoSource::onNewFrame, this, _1, _2));
    d->video->start();
}

void VideoSource::onNewFrame(const void* data, int size)
{
//    auto ts = QDateTime::currentDateTime().toString("hh.mm.ss.zzz");
    MatPtr mat = MatPtr::create(::height, ::width, CV_8UC1);
    memcpy(mat->data, data, ::height * ::width); // read only Y from YUV420.
//    cv::putText(*mat, ts.toStdString(), cvPoint(30,300), cv::FONT_HERSHEY_COMPLEX_SMALL, 4, cv::Scalar(100,200,250), 1, CV_AA);
    d->imageP->publish(mat);

//    static int i = 0;
//    cv::imwrite(QString("/tmp/11/%1.jpg").arg(i).toStdString(), *mat);
//    ++i;
}

void VideoSource::onImageSettingsChanged(const ImageSettings& settings)
{
    d->setBrightness(settings.brightness);
    d->setContrast(settings.contrast);
}

void VideoSource::onConnectionChanged(const data_source::AbstractLinkPtr& link)
{
    if (link)
    {
        d->video->setUdpHost(link->send().address());
        QString path = QString("%1").arg(d->video->port());
        qDebug() << QString("Stream path: host=%1, port=%2")
            .arg(link->send().address().toString())
            .arg(d->video->port());
        d->videoSourceP->publish(path);
    }
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
