#include "sight_model.h"

using domain::SightModel;

class SightModel::Impl
{
public:
    QImage frame;
};

SightModel::SightModel(QObject *parent) :
    QObject(parent),
    d(new Impl)
{

}

SightModel::~SightModel()
{
    delete d;
}

void SightModel::setFrame(const QImage &frame)
{
    d->frame = frame;
    emit frameChanged(frame);
}

QImage SightModel::frame() const
{
    return d->frame;
}

QSize SightModel::size() const
{
    return d->frame.size();
}
