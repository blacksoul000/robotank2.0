#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include "i_exchanger.h"

class I2CSlave : public IExchanger
{
public:
    I2CSlave(uint8_t address, QObject* parent = nullptr);
    ~I2CSlave() override;

    bool sendData(const QByteArray& data) override;

    bool open() override;
    void close() override;
    bool isOpen() const override;

    void onEvent(int event, uint32_t tick);

private:
    struct Impl;
    Impl* d;
};

#endif // I2C_SLAVE_H
