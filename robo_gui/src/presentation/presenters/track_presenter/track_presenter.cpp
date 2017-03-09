#include "track_presenter.h"

#include "robo_model.h"
#include "track_model.h"

#include <QDebug>

using presentation::TrackPresenter;

class TrackPresenter::Impl
{
public:
    domain::RoboModel* model = nullptr;
};

TrackPresenter::TrackPresenter(domain::RoboModel *model, QObject *parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;
    connect(model->track(), &domain::TrackModel::targetRectChanged,
            this, &TrackPresenter::targetRectChanged);
    connect(model->track(), &domain::TrackModel::captureSizeChanged,
            this, &TrackPresenter::captureSizeChanged);
    connect(model->track(), &domain::TrackModel::trackingStatusChanged,
            this, &TrackPresenter::trackingStatusChanged);
}

TrackPresenter::~TrackPresenter()
{
    delete d;
}

void TrackPresenter::onTrackRequest(const QRectF& rect)
{
    d->model->track()->trackRequest(rect);
}

QRectF TrackPresenter::targetRect() const
{
    return d->model->track()->targetRect();
}

QSize TrackPresenter::captureSize() const
{
    return d->model->track()->captureSize();
}

void TrackPresenter::setCaptureFrameRect(const QRectF& rect)
{
    d->model->track()->setCaptureRect(rect);
}

bool TrackPresenter::isTracking() const
{
    return d->model->track()->isTracking();
}
