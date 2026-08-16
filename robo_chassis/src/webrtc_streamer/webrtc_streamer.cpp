#include "webrtc_streamer.h"

#include <QDebug>
#include <QHostAddress>
#include <QUdpSocket>

#ifdef WITH_GST
#include <gst/gst.h>
#include <gst/app/gstappsink.h>


static GstPadProbeReturn bufferProbeProxy(GstPad* pad, GstPadProbeInfo* info, gpointer obj)
{
    return (GstPadProbeReturn)reinterpret_cast< WebRtcStreamer* >(obj)->bufferProbe(pad, info);
}


static gboolean onBusMessageProxy(GstBus* bus, GstMessage* message, void* obj)
{
    return reinterpret_cast< WebRtcStreamer* >(obj)->onBusMessage(bus, message);
}
#endif  // WITH_GST


class WebRtcStreamer::Impl
{
public:
    QHostAddress host = QHostAddress::LocalHost;
    quint16 port = 8554;  // Стандартный порт для WebRTC-стриминга
    quint16 width = 640;  // Оптимальное разрешение для RPi 2
    quint16 height = 480;
    quint8 fps = 30;
    std::function< void(const void*, int) > callback;
    bool isRunning = false;

#ifdef WITH_GST
    GstElement* pipeline = nullptr;
    GstElement* source = nullptr;
    GstElement* encoder = nullptr;
    GstElement* payloader = nullptr;
    GstElement* sink = nullptr;
#endif  // WITH_GST
};


WebRtcStreamer::WebRtcStreamer(quint16 port, quint16 width, quint16 height, quint8 fps, QObject* parent):
    QObject(parent),
    d(new Impl)
{
    d->port = port;
    d->width = width;
    d->height = height;
    d->fps = fps;

#ifdef WITH_GST
    gst_init_check(nullptr, nullptr, nullptr);

    // Пайплайн с аппаратным кодированием H.264 для Raspberry Pi 2
    // omxh264enc использует аппаратный блок VPU для минимальной нагрузки на CPU
    QString pipelineStr;
    
#ifdef PICAM
    // Для камеры Raspberry Pi
    pipelineStr = QString(
        "rpicamsrc bitrate=1000000 keyframe-maxdist=30 name=source ! "
        "video/x-h264,width=%1,height=%2,framerate=%3/1 ! "
        "omxh264enc control-rate=low target-bitrate=800000 preset=highpower ! "
        "video/x-h264,profile=baseline,stream-format=byte-stream,alignment=nal ! "
        "rtph264pay pt=96 config-interval=1 name=payloader ! "
        "udpsink host=%4 port=%5 name=udpsink async=false")
        .arg(d->width).arg(d->height).arg(d->fps)
        .arg(d->host.toString()).arg(d->port);
#else
    // Для USB-камеры или тестового источника
    pipelineStr = QString(
        "v4l2src device=/dev/video0 do-timestamp=true name=source ! "
        "video/x-raw,width=%1,height=%2,framerate=%3/1 ! "
        "omxh264enc control-rate=low target-bitrate=800000 preset=highpower ! "
        "video/x-h264,profile=baseline,stream-format=byte-stream,alignment=nal ! "
        "rtph264pay pt=96 config-interval=1 name=payloader ! "
        "udpsink host=%4 port=%5 name=udpsink async=false")
        .arg(d->width).arg(d->height).arg(d->fps)
        .arg(d->host.toString()).arg(d->port);
#endif  // PICAM

    qDebug() << "WebRTC Pipeline:" << pipelineStr;
    
    d->pipeline = gst_parse_launch(pipelineStr.toLocal8Bit().data(), nullptr);
    if (!d->pipeline)
    {
        qWarning() << Q_FUNC_INFO << "Failed to create pipeline";
        return;
    }
    
    d->source = gst_bin_get_by_name(GST_BIN(d->pipeline), "source");
    d->encoder = gst_bin_get_by_name(GST_BIN(d->pipeline), "omxh264enc0");
    d->payloader = gst_bin_get_by_name(GST_BIN(d->pipeline), "payloader");
    d->sink = gst_bin_get_by_name(GST_BIN(d->pipeline), "udpsink");
    
    // Добавляем probe для перехвата кадров
    if (d->source)
    {
        GstPad* pad = gst_element_get_static_pad(d->source, "src");
        if (pad)
        {
            gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, bufferProbeProxy, this, nullptr);
            gst_object_unref(pad);
        }
    }

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(d->pipeline));
    gst_bus_add_watch(bus, onBusMessageProxy, this);
    gst_object_unref(bus);
    
    qDebug() << "WebRtcStreamer initialized successfully";
#endif  // WITH_GST
}

WebRtcStreamer::~WebRtcStreamer()
{
    stop();
    
#ifdef WITH_GST
    if (d->sink) gst_object_unref(d->sink);
    if (d->payloader) gst_object_unref(d->payloader);
    if (d->encoder) gst_object_unref(d->encoder);
    if (d->source) gst_object_unref(d->source);
    if (d->pipeline) gst_object_unref(d->pipeline);
#endif  // WITH_GST

    delete d;
}

void WebRtcStreamer::setUdpHost(const QHostAddress& host)
{
    if (host == d->host) return;
    d->host = host;
    
    qDebug() << "Setting UDP host to:" << d->host.toString();

#ifdef WITH_GST
    if (!d->pipeline)
    {
        qWarning() << Q_FUNC_INFO << "Pipeline is empty";
        return;
    }
    
    // Пересоздаем пайплайн с новым хостом
    bool wasRunning = d->isRunning;
    if (wasRunning)
    {
        stop();
    }
    
    // Обновляем адрес в udpsink
    if (d->sink)
    {
        g_object_set(d->sink, "host", d->host.toString().toLocal8Bit().data(), NULL);
    }
    
    if (wasRunning)
    {
        start();
    }
#endif  // WITH_GST
}

quint16 WebRtcStreamer::port() const
{
    return d->port;
}

void WebRtcStreamer::start()
{
    if (d->isRunning)
    {
        qWarning() << Q_FUNC_INFO << "Already running";
        return;
    }
    
#ifdef WITH_GST
    if (!d->pipeline)
    {
        qWarning() << Q_FUNC_INFO << "Pipeline is empty";
        return;
    }
    
    qDebug() << "Starting WebRTC stream...";
    GstStateChangeReturn ret = gst_element_set_state(d->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        qWarning() << Q_FUNC_INFO << "Failed to start pipeline";
        return;
    }
    
    d->isRunning = true;
    qDebug() << "WebRTC stream started on port" << d->port;
#else
    qWarning() << Q_FUNC_INFO << "GStreamer not available";
#endif  // WITH_GST
}

void WebRtcStreamer::stop()
{
    if (!d->isRunning)
    {
        return;
    }
    
#ifdef WITH_GST
    if (d->pipeline)
    {
        qDebug() << "Stopping WebRTC stream...";
        gst_element_set_state(d->pipeline, GST_STATE_NULL);
    }
    d->isRunning = false;
    qDebug() << "WebRTC stream stopped";
#endif  // WITH_GST
}

void WebRtcStreamer::setDataCallback(const std::function< void(const void* data, int size) >& callback)
{
    d->callback = callback;
}

int WebRtcStreamer::bufferProbe(GstPad* pad, GstPadProbeInfo* info)
{
#ifdef WITH_GST
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buffer && d->callback)
    {
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_READ))
        {
            d->callback(map.data, map.size);
            gst_buffer_unmap(buffer, &map);
        }
    }

    return GST_PAD_PROBE_OK;
#else
    return 0;
#endif  // WITH_GST
}


bool WebRtcStreamer::onBusMessage(GstBus* bus, GstMessage* message)
{
#ifdef WITH_GST
    Q_UNUSED(bus)

    switch (GST_MESSAGE_TYPE(message))
    {
        case GST_MESSAGE_ERROR:
        {
            GError* error;
            gst_message_parse_error(message, &error, nullptr);
            qWarning() << "*** GStreamer ERROR ***"
                       << gst_element_get_name(message->src)
                       << ":" << error->message;
            g_error_free(error);
            break;
        }
        case GST_MESSAGE_WARNING:
        {
            GError* error;
            gst_message_parse_warning(message, &error, nullptr);
            qWarning() << "*** GStreamer WARNING ***"
                       << gst_element_get_name(message->src)
                       << ":" << error->message;
            g_error_free(error);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
        {
            if (message->src == GST_OBJECT(d->pipeline))
            {
                GstState oldState, newState;
                gst_message_parse_state_changed(message, &oldState, &newState, nullptr);
                qDebug() << "Pipeline state changed from" 
                         << gst_element_state_get_name(oldState)
                         << "to" << gst_element_state_get_name(newState);
            }
            break;
        }
        case GST_MESSAGE_ELEMENT:
        {
            // Обработка событий RTSP/WebRTC
            break;
        }
        default:
            break;
    }
#endif  // WITH_GST
    return TRUE;
}
