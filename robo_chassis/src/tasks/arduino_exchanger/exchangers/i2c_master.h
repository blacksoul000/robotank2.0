#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#include <i_exchanger.h>

class I2CMaster : public IExchanger
{
public:
	I2CMaster(const QString& device, int packageSize, QObject* parent = nullptr);
	virtual ~I2CMaster();

	bool sendData(const QByteArray& data) override;
	bool readData();

	bool open() override;
	void close() override;
	bool isOpen() const override;

private:
	class Impl;
	Impl* d;
};

#endif // I2C_MASTER_H_
