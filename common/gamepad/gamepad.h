#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <QObject>

class Gamepad : public QObject
{
    Q_OBJECT
public:
    explicit Gamepad(const QString& device, QObject* parent = nullptr);
    ~Gamepad();

    void execute();

    QVector< short > axes() const;
    quint16 buttons() const;
    quint8 axesCount() const;

    int capacity() const;
    bool isConnected() const;
    bool isCharging() const;

signals:
	void capacityChanged(int capacity);
	void chargingChanged(bool charging);
	void connectedChanged(bool connected);

private:
    void readData();
    bool open();
    void close();

private slots:
    void onCapacityChanged();
    void onStatusChanged();

private:
    class Impl;
    Impl* d;
};

#endif // GAMEPAD_H
