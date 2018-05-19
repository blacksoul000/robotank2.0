#ifndef UART_H_
#define UART_H_

#include <i_exchanger.h>

class Uart : public IExchanger
{
public:
	Uart(const QString& device, int packageSize, QObject* parent = nullptr);
	virtual ~Uart();

	bool sendData(const QByteArray& data) override;

	bool open() override;
	void close() override;
	bool isOpen() const override;

private slots:
	void onReadyRead();

private:
	class Impl;
	Impl* d;
};

#endif // UART_H_
