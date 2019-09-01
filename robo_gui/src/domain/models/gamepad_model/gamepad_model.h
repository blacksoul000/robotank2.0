#ifndef GAMEPAD_MODEL_H
#define GAMEPAD_MODEL_H

#include <QObject>

namespace domain
{
	class GamepadModel : public QObject
	{
		Q_OBJECT
	public:

		// NOTE - Wired and wireless connections differs in axis count and order
		// TODO - Axes setup on gui? and move enum to gamepad.cpp
	    enum Axes // Wired
	    {
	        X1 = 0,
	        Y1 = 1,
	        X2 = 3, // 2,
	        Y2 = 4, // 5,
	        DigitalX = 6,
	        DigitalY = 7,
	    };

		GamepadModel(QObject* parent = nullptr);
		~GamepadModel();

		void execute();

		int capacity() const;
		bool isCharging() const;
		bool isConnected() const;

		void setCharging(bool charging);
		void setCapacity(int capacity);
		void setConnected(bool connected);

	    QVector< short > axes() const;
	    quint16 buttons() const;

	signals:
		void capacityChanged(int capacity);
		void chargingChanged(bool charging);
		void connectedChanged(bool connected);
		void joyChanged();

	protected:
		void timerEvent(QTimerEvent* event) override;

	private:
		class Impl;
		Impl* d;
	};
}  // namespace domain

#endif // GAMEPAD_MODEL_H
