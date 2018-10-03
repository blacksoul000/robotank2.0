#include "rtsp_server.h"

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <gst/base/gstbasesink.h>

#include <iostream>

using rtsp_server::RtspServer;

namespace
{
    const std::string streamName("live");
    const uint16_t port = 8554;
} // namespace

static GstPadProbeReturn bufferProbeProxy(GstPad* pad, GstPadProbeInfo* info, gpointer obj);

class RtspServer::Impl
{
public:
    GstRTSPServer* server = nullptr;
    GstRTSPMediaFactory* factory = nullptr;
    guint gstServerId = 0;

    std::function< void(const void* data, int size) > callback;
};

RtspServer::RtspServer(const std::string& pipeline) :
    d(new Impl)
{
    gst_init_check(nullptr, nullptr, nullptr);

    /* create a server instance */
    d->server = gst_rtsp_server_new ();

    gst_rtsp_server_set_service(d->server, std::to_string(::port).c_str());

    /* get the mount points for this server, every server has a default object
     * that be used to map uri mount points to media factories */
    GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points (d->server);

    /* make a media factory for a test stream. The default media factory can use
     * gst-launch syntax to create pipelines.
     * any launch line works as long as it contains elements named pay%d. Each
     * element with pay%d names will be a stream */
    d->factory = gst_rtsp_media_factory_new ();
    gst_rtsp_media_factory_set_shared (d->factory, TRUE);
    gst_rtsp_media_factory_set_launch (d->factory, std::string("( " + pipeline + " )").c_str());

    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory (mounts, std::string("/" + ::streamName).c_str(), d->factory);

    /* don't need the ref to the mapper anymore */
    g_object_unref (mounts);

    /* notify when our media is ready, This is called whenever someone asks for
     * the media and a new pipeline with our sink is created */
    g_signal_connect(d->factory, "media-configure",
                     G_CALLBACK(&RtspServer::mediaConfigureProxy), this);
}

RtspServer::~RtspServer()
{
    this->stop();

    gst_object_unref(d->factory);
    gst_object_unref(d->server);
    gst_deinit();
    delete d;
}

gboolean RtspServer::onTimeoutProxy(gpointer obj)
{
    return reinterpret_cast< RtspServer* >(obj)->onTimeout();
}

gboolean RtspServer::onTimeout()
{
    GstRTSPSessionPool* pool = gst_rtsp_server_get_session_pool (d->server);
    gst_rtsp_session_pool_cleanup (pool);
    g_object_unref (pool);

    return TRUE;
}

void RtspServer::mediaConfigureProxy(GstRTSPMediaFactory* factory, GstRTSPMedia* media,
                                     gpointer obj)
{
    reinterpret_cast< RtspServer* >(obj)->mediaConfigure(factory, media);
}

void RtspServer::mediaConfigure(GstRTSPMediaFactory*, GstRTSPMedia* media)
{
    GstElement* element = gst_rtsp_media_get_element(media);

    GstElement* src = gst_bin_get_by_name_recurse_up (GST_BIN (element), "src");
    gst_object_unref(element);

    if (!src)
    {
        std::cerr << "Failed to get source with 'name=src' property set.";
        return;
    }

    GstPad* pad = gst_element_get_static_pad (src, "src");
    gst_object_unref(src);

    gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER, bufferProbeProxy, this, nullptr);
    gst_object_unref(pad);
}

int RtspServer::bufferProbe(GstPad* pad, GstPadProbeInfo* info)
{
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER (info);
    if (buffer)
    {
        GstMapInfo map;
        gst_buffer_map (buffer, &map, GST_MAP_READ);
        d->callback(map.data, map.size);
        gst_buffer_unmap (buffer, &map);
    }

    return GST_PAD_PROBE_OK;
}

void RtspServer::setDataCallback(const std::function< void(const void* data, int size) >& cb)
{
    d->callback = cb;
}

void RtspServer::start()
{
    /* attach the server to the default maincontext */
    d->gstServerId = gst_rtsp_server_attach(d->server, nullptr);

    /* add a timeout for the session cleanup */
    g_timeout_add_seconds (2, GSourceFunc(&RtspServer::onTimeoutProxy), this);
}

void RtspServer::stop()
{
    if (!d->gstServerId == 0) return;
    g_source_remove(d->gstServerId);
}

uint16_t RtspServer::port() const
{
    return ::port;
}

std::string RtspServer::streamName() const
{
    return ::streamName;
}

GstPadProbeReturn bufferProbeProxy(GstPad* pad, GstPadProbeInfo* info, gpointer obj)
{
    return (GstPadProbeReturn)reinterpret_cast< RtspServer* >(obj)->bufferProbe(pad, info);
}
