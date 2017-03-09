#include "frame_presenter.h"

#include "robo_model.h"
#include "sight_model.h"

#include <QVideoSurfaceFormat>
#include <QAbstractVideoSurface>
#include <QDebug>

using presentation::FramePresenter;

class FramePresenter::Impl
{
public:
    domain::RoboModel* model = nullptr;
    QAbstractVideoSurface* surface = nullptr;
    QVideoSurfaceFormat format;
    bool hasFrame = false;
};

FramePresenter::FramePresenter(domain::RoboModel *model, QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;
    connect(model->sight(), &domain::SightModel::frameChanged,
            this, &FramePresenter::onFrameChanged);
}

FramePresenter::~FramePresenter()
{
    delete d;
}

QAbstractVideoSurface *FramePresenter::videoSurface() const
{
    return d->surface;
}

void FramePresenter::setVideoSurface( QAbstractVideoSurface* s )
{
    this->closeSurface();
    d->surface = s;
}

void FramePresenter::closeSurface()
{
    if (d->surface && d->surface->isActive()) d->surface->stop();
}

void FramePresenter::onFrameChanged(const QImage& frame)
{
    if (!d->surface) return;
    if (d->hasFrame != !frame.isNull())
    {
        d->hasFrame = !frame.isNull();
        emit hasFrameChanged();
    }
    if (frame.isNull()) return;

    QVideoFrame::PixelFormat pixelFormat =
            QVideoFrame::pixelFormatFromImageFormat(frame.format());

    if (d->format.frameHeight() != frame.height()
            || d->format.frameWidth() != frame.width())
    {
        this->closeSurface();
        d->format = QVideoSurfaceFormat(frame.size(), pixelFormat);
//        d->format = QVideoSurfaceFormat(frame.size(), QVideoFrame::Format_YUV420P);
        d->surface->start(d->format);
    }

//    qDebug() << "1";
//    QVideoFrame videoFrame(frame.byteCount(), frame.size(), frame.bytesPerLine(), QVideoFrame::Format_YUV420P);
//    qDebug() << "2";
//    videoFrame.map(QAbstractVideoBuffer::WriteOnly);
//    qDebug() << "3" << frame.size() << frame.byteCount() << frame.format();
//    memcpy(videoFrame.bits(), frame.bits(), frame.byteCount());
//    qDebug() << "4";
//    videoFrame.unmap();
//    qDebug() << "5";

//    d->surface->present(videoFrame);
//    qDebug() << "6";
    d->surface->present(QVideoFrame(frame));
}

bool FramePresenter::hasFrame() const
{
    return d->surface && d->hasFrame;
}
