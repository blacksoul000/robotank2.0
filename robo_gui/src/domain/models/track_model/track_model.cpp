#include "track_model.h"

#include <QVector>
#include <QSize>
#include <QDebug>

using domain::TrackModel;

class TrackModel::Impl
{
public:
    QRectF targetRect;
    QRectF captureRect;
    QVector< QSize > sizes = {QSize(32, 32), QSize(64, 64), QSize(128, 128), QSize(256, 256)};
    int index = 0;
    bool tracking = false;
};

TrackModel::TrackModel(QObject* parent) :
    QObject(parent),
    d(new Impl)
{
}

TrackModel::~TrackModel()
{
    delete d;
}

void TrackModel::setTargetRect(const QRectF& rect)
{
    if (rect == d->targetRect) return;
    d->targetRect = rect;

    emit targetRectChanged(rect);
}

QRectF TrackModel::targetRect() const
{
    return d->targetRect;
}

void TrackModel::nextCaptureSize()
{
    ++d->index;
    if (d->index == d->sizes.length()) d->index = 0;
    emit captureSizeChanged(this->captureSize());
}

QSize TrackModel::captureSize() const
{
    return d->sizes.at(d->index);
}

void TrackModel::setCaptureRect(const QRectF& rect)
{
    d->captureRect = rect;
}

QRectF TrackModel::captureRect() const
{
    return d->captureRect;
}

bool TrackModel::isTracking() const
{
    return d->tracking;
}

void TrackModel::setTracking(bool tracking)
{
    if (d->tracking == tracking) return;
    d->tracking = tracking;
    d->targetRect = QRectF();
    emit trackingStatusChanged(tracking);
}
