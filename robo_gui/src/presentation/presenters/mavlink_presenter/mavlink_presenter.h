#ifndef MAVLINK_PRESENTER_H
#define MAVLINK_PRESENTER_H

#include <QObject>
#include <QList>

namespace domain
{
    class RoboModel;
}

namespace presentation
{
    class MavlinkPresenter : public QObject
    {
        Q_OBJECT
    public:
        Q_PROPERTY(QList< QObject* > vehicles READ vehicles NOTIFY vehiclesChanged);
        Q_PROPERTY(QObject* currentVehicle READ currentVehicle 
                   WRITE setCurrentVehicle NOTIFY currentVehicleChanged);

        MavlinkPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~MavlinkPresenter() override;

        QObject* currentVehicle() const;
        QList< QObject* > vehicles() const;

        void setCurrentVehicle(QObject* vehicle);

        void onVehiclesChanged();
        void onCurrentVehicleChanged();

    signals:
        void vehiclesChanged();
        void currentVehicleChanged();

    private:
        class Impl;
        Impl* d;
    };
}

#endif //MAVLINK_PRESENTER_H
