#ifndef BLUETOOTH_PAIR_REQUEST_H
#define BLUETOOTH_PAIR_REQUEST_H

#include <QString>

struct BluetoothPairRequest
{
    QString device;
    bool pair;
};

#endif // BLUETOOTH_PAIR_REQUEST_H
