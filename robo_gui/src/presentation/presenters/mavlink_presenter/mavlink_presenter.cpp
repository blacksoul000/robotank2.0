#include "mavlink_presenter.h"

#include "robo_model.h"
#include "mavlink_exchanger.h"
#include "vehicle_view.h"

#include <QDebug>

using presentation::MavlinkPresenter;
using presentation::VehicleView;

class MavlinkPresenter::Impl
{
public:
    domain::RoboModel* model = nullptr;
    QList< QObject* > vehicles;
    QObject* currentVehicle = nullptr;
};

MavlinkPresenter::MavlinkPresenter(domain::RoboModel* model, QObject* parent):
    QObject(parent),
    d(new Impl)
{
    d->model = model;
    connect(model->mavlink(), &domain::MavlinkExchanger::vehiclesChanged,
            this, &MavlinkPresenter::onVehiclesChanged);
    connect(model->mavlink(), &domain::MavlinkExchanger::currentVehicleChanged,
            this, &MavlinkPresenter::onCurrentVehicleChanged);
    this->onVehiclesChanged();
    this->onCurrentVehicleChanged();
}

MavlinkPresenter::~MavlinkPresenter()
{
    delete d;
}

QList< QObject* > MavlinkPresenter::vehicles() const
{
    return d->vehicles;
}

QObject* MavlinkPresenter::currentVehicle() const
{
    return d->currentVehicle;
}

void MavlinkPresenter::setCurrentVehicle(QObject* vehicle)
{
    VehicleView* view = dynamic_cast< VehicleView* >(vehicle);

    if (view) d->model->mavlink()->setCurrentVehicle(view->vehicle());
}

void MavlinkPresenter::onVehiclesChanged()
{
    auto old = d->vehicles;
    d->vehicles.clear();

    for (const auto& item: d->model->mavlink()->vehicles().values())
    {
        d->vehicles.append(new VehicleView(item));
    }
    qDeleteAll(old);
    emit vehiclesChanged();
}

void MavlinkPresenter::onCurrentVehicleChanged()
{
    auto v = d->model->mavlink()->currentVehicle();
    if (v)
    {
        auto it = std::find_if(d->vehicles.begin(), d->vehicles.end(), [&, v](QObject* vehicleObj){
            const auto* view = qobject_cast< VehicleView* >(vehicleObj);
            return (view && view->vehicle() == v);
        });

        if (it != d->vehicles.end())
        {
            d->currentVehicle = *it;
        }
    }

    emit currentVehicleChanged();
}