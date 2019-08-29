#include "gamepad_controller.h"

//msgs
#include "joy_axes.h"

#include "pub_sub.h"
#include "gamepad.h"

//Qt
#include <QScopedPointer>
#include <QDebug>

class GamepadController::Impl
{
public:
    QScopedPointer< Gamepad > gamepad;

    JoyAxes axes;

    Publisher< bool >* joyStatusP = nullptr;
    Publisher< JoyAxes >* axesP = nullptr;
    Publisher< quint16 >* buttonsP = nullptr;
    Publisher< quint8 >* capacityP = nullptr;
    Publisher< bool >* chargingP = nullptr;
};

GamepadController::GamepadController() :
    ITask(),
    d(new Impl)
{
	d->gamepad.reset(new Gamepad("/dev/input/js0", this));
	connect(d->gamepad.data(), &Gamepad::capacityChanged,
			this, &GamepadController::onCapacityChanged);
	connect(d->gamepad.data(), &Gamepad::chargingChanged,
			this, &GamepadController::onChargingChanged);
	connect(d->gamepad.data(), &Gamepad::connectedChanged,
			this, &GamepadController::onConnectedChanged);

    d->joyStatusP = PubSub::instance()->advertise< bool >("joy/status");
    d->axesP = PubSub::instance()->advertise< JoyAxes >("joy/axes");
    d->buttonsP = PubSub::instance()->advertise< quint16 >("joy/buttons");
    d->capacityP = PubSub::instance()->advertise< quint8 >("joy/capacity");
    d->chargingP = PubSub::instance()->advertise< bool >("joy/charging");
}

GamepadController::~GamepadController()
{
	d->gamepad.reset();

    delete d->joyStatusP;
    delete d->axesP;
    delete d->buttonsP;
    delete d->capacityP;
    delete d->chargingP;
    delete d;
}

void GamepadController::execute()
{
	d->gamepad->execute();
	if (!d->gamepad->isConnected()) return;

	const auto& axes = d->gamepad->axes();
	d->axes.x1 = axes.at(Axes::DigitalX)
							? axes.at(Axes::DigitalX) : axes.at(Axes::X1);
	d->axes.y1 = axes.at(Axes::DigitalY)
				    		? -axes.at(Axes::DigitalY) : -axes.at(Axes::Y1);
	d->axes.x2 = axes.at(Axes::X2);
	d->axes.y2 = axes.at(Axes::Y2);

	d->axesP->publish(d->axes);
	d->buttonsP->publish(d->gamepad->buttons());
}

void GamepadController::onConnectedChanged(bool connected)
{
    d->joyStatusP->publish(connected);
}

void GamepadController::onCapacityChanged(int capacity)
{
    d->capacityP->publish(capacity);
}

void GamepadController::onChargingChanged(bool charging)
{
    d->chargingP->publish(charging);
}
