#include "track_model.h"

#include <QMutex>
#include <QMutexLocker>
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

    QMutex mutex;
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
    QMutexLocker locker(&d->mutex);
    if (rect == d->targetRect) return;
    d->targetRect = rect;
    locker.unlock();

    emit targetRectChanged(rect);
}

QRectF TrackModel::targetRect() const
{
    QMutexLocker locker(&d->mutex);
    return d->targetRect;
}

void TrackModel::nextCaptureSize()
{
    QMutexLocker locker(&d->mutex);
    ++d->index;
    if (d->index == d->sizes.length()) d->index = 0;
    locker.unlock();

    emit captureSizeChanged(this->captureSize());
}

QSize TrackModel::captureSize() const
{
    QMutexLocker locker(&d->mutex);
    return d->sizes.at(d->index);
}

void TrackModel::setCaptureRect(const QRectF& rect)
{
    QMutexLocker locker(&d->mutex);
    d->captureRect = rect;
}

QRectF TrackModel::captureRect() const
{
    QMutexLocker locker(&d->mutex);
    return d->captureRect;
}

bool TrackModel::isTracking() const
{
    QMutexLocker locker(&d->mutex);
    return d->tracking;
}

void TrackModel::setTracking(bool tracking)
{
    QMutexLocker locker(&d->mutex);
    if (d->tracking == tracking) return;
    d->tracking = tracking;
    d->targetRect = QRectF();
    locker.unlock();

    emit trackingStatusChanged(tracking);
}
