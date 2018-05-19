#ifndef I_EXCHANGER_H_
#define I_EXCHANGER_H_

#include <QObject>

class IExchanger : public QObject
{
	Q_OBJECT

public:
	IExchanger(QObject* parent = nullptr);
	virtual ~IExchanger() = default;

	virtual bool sendData(const QByteArray& data) = 0;

	virtual bool open() = 0;
	virtual void close() = 0;
	virtual bool isOpen() const = 0;

signals:
	void dataAvailable(const QByteArray& data);
};

#endif // I_EXCHANGER_H_
