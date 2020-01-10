#include "mavlink_presenter.h"

#include "robo_model.h"
#include "mavlink_exchanger.h"
#include "vehicle.h"

#include <QDebug>

using presentation::MavlinkPresenter;

class MavlinkPresenter::Impl
{
public:
    domain::RoboModel* model = nullptr;
    QList< domain::VehiclePtr > vehicles;
};

MavlinkPresenter::MavlinkPresenter(domain::RoboModel* model, QObject* parent):
    QObject(parent),
    d(new Impl)
{
    d->model = model;
    connect(model->mavlink(), &domain::MavlinkExchanger::vehiclesChanged,
            this, &MavlinkPresenter::vehiclesChanged);
    connect(model->mavlink(), &domain::MavlinkExchanger::currentVehicleChanged,
            this, &MavlinkPresenter::currentVehicleChanged);
}

MavlinkPresenter::~MavlinkPresenter()
{
    delete d;
}

QList< QObject* > MavlinkPresenter::vehicles() const
{
    d->vehicles = d->model->mavlink()->vehicles().values();  // Store pointer

    QList< QObject* > result;
    for (const auto& item: d->vehicles)
    {
        result.append(item.data());
    }
    return result;
}

QObject* MavlinkPresenter::currentVehicle() const
{
    return d->model->mavlink()->currentVehicle().data();
}

void MavlinkPresenter::setCurrentVehicle(QObject* vehicle)
{
    domain::VehiclePtr ptr;
    if (vehicle)
    {
        auto it = std::find_if(d->vehicles.cbegin(), d->vehicles.cend(), 
            [vehicle](const domain::VehiclePtr& rhs){
                return rhs.data() == vehicle;
        });
        if (it != d->vehicles.cend())
        {
            ptr = *it;
        }
    }
    d->model->mavlink()->setCurrentVehicle(ptr);
}