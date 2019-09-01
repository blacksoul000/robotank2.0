#ifndef I_IMU_H
#define I_IMU_H

class IImu
{
public:
    virtual ~IImu() {}

    virtual bool init() = 0;
    virtual bool isOnline() const = 0;
    virtual bool isReady() const = 0;
    virtual void readData() = 0;

    virtual float yaw() const = 0;
    virtual float pitch() const = 0;
    virtual float roll() const = 0;
};

#endif // I_IMU_H
