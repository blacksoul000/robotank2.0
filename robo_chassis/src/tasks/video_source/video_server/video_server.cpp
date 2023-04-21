#include "video_server.h"

#include <QDebug>

#ifdef WITH_GST
#include <gst/gst.h>


static GstPadProbeReturn bufferProbeProxy(GstPad* pad, GstPadProbeInfo* info, gpointer obj)
{
    return (GstPadProbeReturn)reinterpret_cast< VideoServer* >(obj)->bufferProbe(pad, info);
}
#endif  // WITH_GST


class VideoServer::Impl
{
public:
    QHostAddress host = QHostAddress::LocalHost;
    quint16 port = 5000;
    quint16 width = 1024;
    quint16 height = 768;
    quint8 fps = 25;
    std::function< void(const void*, int) > callback;

#ifdef WITH_GST
    GstElement* pipeline = nullptr;
    GstElement* source = nullptr;
    GstElement* sink = nullptr;
#endif  // WITH_GST
};


VideoServer::VideoServer(quint16 port, quint16 width, quint16 height, quint8 fps, QObject* parent):
    QObject(parent),
    d(new Impl)
{
    d->port = port;
    d->width = width;
    d->height = height;
    d->fps = fps;

#ifdef WITH_GST
    gst_init_check(nullptr, nullptr, nullptr);

    d->pipeline = gst_parse_launch(QString(
#ifdef PICAM
        "rpicamsrc do-timestamp=true name=source ! "
#else
        "videotestsrc do-timestamp=true name=source ! "
#endif  // PICAM
        "video/x-raw,width=%1,height=%2,framerate=%3/1 ! "
        "omxh264enc control-rate=1 target-bitrate=1000000 ! "
        "video/x-h264,profile=baseline,low-latency=true,speed-preset=ultrafast ! "
        "rtph264pay pt=96 ! udpsink host=%4 port=%5 name=udpsink")
            .arg(d->width).arg(d->height).arg(fps).arg(d->host.toString())
            .arg(d->port).toLocal8Bit().data(), nullptr);
    d->source = gst_bin_get_by_name(GST_BIN(d->pipeline), "source");
    d->sink = gst_bin_get_by_name(GST_BIN(d->pipeline), "udpsink");
    
    GstPad* pad = gst_element_get_static_pad(d->source, "src");
    gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER, bufferProbeProxy, this, nullptr);
    gst_object_unref(pad);
#endif  // WITH_GST
}

VideoServer::~VideoServer()
{
#ifdef WITH_GST
    gst_element_set_state(d->pipeline, GST_STATE_NULL);

    gst_object_unref(d->sink);
    gst_object_unref(d->source);
    gst_object_unref(d->pipeline);
#endif  // WITH_GST

    delete d;
}

void VideoServer::setUdpHost(const QHostAddress& host)
{
    d->host = host;

#ifdef WITH_GST
    gst_element_set_state(d->pipeline, GST_STATE_PAUSED);
    g_object_set(d->sink, "host", d->host.toString().toLocal8Bit().data(), NULL);
    gst_element_set_state(d->pipeline, GST_STATE_PLAYING);
#endif  // WITH_GST
}

quint16 VideoServer::port() const
{
    return d->port;
}

void VideoServer::start()
{
#ifdef WITH_GST
    gst_element_set_state(d->pipeline, GST_STATE_PLAYING);
#endif  // WITH_GST
}

void VideoServer::setDataCallback(const std::function< void(const void* data, int size) >& callback)
{
    d->callback = callback;
}

int VideoServer::bufferProbe(GstPad* pad, GstPadProbeInfo* info)
{
#ifdef WITH_GST
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER (info);
    if (buffer)
    {
        GstMapInfo map;
        gst_buffer_map (buffer, &map, GST_MAP_READ);
        d->callback(map.data, map.size);
        gst_buffer_unmap (buffer, &map);
    }

    return GST_PAD_PROBE_OK;
#else
    return 0;
#endif  // WITH_GST
}
