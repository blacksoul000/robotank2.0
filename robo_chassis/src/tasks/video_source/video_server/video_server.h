#ifndef VIDEO_SERVER_H
#define VIDEO_SERVER_H

#include <QObject>
#include <QHostAddress>

#include <functional>


typedef struct _GstPad GstPad;
typedef struct _GstPadProbeInfo GstPadProbeInfo;
typedef struct _GstBus GstBus;
typedef struct _GstMessage GstMessage;


class VideoServer : public QObject
{
public:
    VideoServer(quint16 port, quint16 width, quint16 height, quint8 fps, QObject* parent);
    virtual ~VideoServer();

    void setUdpHost(const QHostAddress& host);

    quint16 port() const;
    void start();

    void setDataCallback(const std::function< void(const void* data, int size) >& callback);
    int bufferProbe(GstPad* pad, GstPadProbeInfo* info);
    bool onBusMessage(GstBus* bus, GstMessage* message);

private:
    class Impl;
    Impl* d;
};

#endif // VIDEO_SERVER_H