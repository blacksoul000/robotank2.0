#include "i2c_slave.h"

#include <pigpio.h>
#include <iostream>
#include <functional>

//https://raspberrypi.stackexchange.com/questions/76109/raspberry-as-an-i2c-slave?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa

static void onEventProxy(int event, uint32_t tick, void* obj)
{
    reinterpret_cast< I2CSlave* >(obj)->onEvent(event, tick);
}

struct I2CSlave::Impl
{
    bsc_xfer_t xfer; // Struct to control data flow

    uint8_t address = 0x42;
    bool isOpen = false;

    uint32_t getControlBits(uint32_t address /* max 127 */, bool open);
};

I2CSlave::I2CSlave(uint8_t address, QObject* parent) :
    IExchanger(parent),
    d(new Impl)
{
    d->address = address;
}

I2CSlave::~I2CSlave()
{
    delete d;
}

bool I2CSlave::open()
{
    // Close old device (if any)
    d->xfer.control = d->getControlBits(d->address, false); // To avoid conflicts when restarting
    bscXfer(&d->xfer);
    // Set I2C slave Address
    d->xfer.control = d->getControlBits(d->address, true);
    int status = bscXfer(&d->xfer); // Should now be visible in I2C-Scanners
    d->xfer.rxCnt = 0;

    if (status >= 0 )
    {
        std::cout << "Opened slave\n";
        eventSetFuncEx(PI_EVENT_BSC, &onEventProxy, this);

        d->isOpen = true;
        return true;
    }
    
    return false;
}

void I2CSlave::close()
{
    d->xfer.control = d->getControlBits(d->address, false);
    bscXfer(&d->xfer);
}

bool I2CSlave::isOpen() const
{
    return d->isOpen;
}

bool I2CSlave::sendData(const QByteArray& data)
{
    memcpy(d->xfer.txBuf, data.data(), data.size());
    d->xfer.txCnt = data.size();
    auto res = bscXfer(&d->xfer);
    std::cout << "res = " << res << std::endl;
    return res >= 0;
}

void I2CSlave::onEvent(int, uint32_t)
{
    bscXfer(&d->xfer);
    if(d->xfer.rxCnt > 0)
    {
        emit dataAvailable(QByteArray(d->xfer.rxBuf, d->xfer.rxCnt));
    }
}

uint32_t I2CSlave::Impl::getControlBits(uint32_t address /* max 127 */, bool open)
{
    /*
    Excerpt from http://abyz.me.uk/rpi/pigpio/cif.html#bscXfer regarding the control bits:

    22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    a  a  a  a  a  a  a  -  -  IT HC TF IR RE TE BK EC ES PL PH I2 SP EN

    Bits 0-13 are copied unchanged to the BSC CR register. See pages 163-165 of the Broadcom
    peripherals document for full details.

    aaaaaaa defines the I2C slave address (only relevant in I2C mode)
    IT  invert transmit status flags
    HC  enable host control
    TF  enable test FIFO
    IR  invert receive status flags
    RE  enable receive
    TE  enable transmit
    BK  abort operation and clear FIFOs
    EC  send control register as first I2C byte
    ES  send status register as first I2C byte
    PL  set SPI polarity high
    PH  set SPI phase high
    I2  enable I2C mode
    SP  enable SPI mode
    EN  enable BSC peripheral
    */

    // Flags like this: 0b/*IT:*/0/*HC:*/0/*TF:*/0/*IR:*/0/*RE:*/0/*TE:*/0/*BK:*/0/*EC:*/0/*ES:*/0/*PL:*/0/*PH:*/0/*I2:*/0/*SP:*/0/*EN:*/0;

    uint32_t flags;
    if(open)
        flags = /*RE:*/ (1 << 9) | /*TE:*/ (1 << 8) | /*I2:*/ (1 << 2) | /*EN:*/ (1 << 0);
    else // Close/Abort
        flags = /*BK:*/ (1 << 7) | /*I2:*/ (0 << 2) | /*EN:*/ (0 << 0);

    return (address << 16 /*= to the start of significant bits*/) | flags;
}
