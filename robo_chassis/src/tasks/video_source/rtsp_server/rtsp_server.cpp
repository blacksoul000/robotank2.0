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

class RtspServer::Impl
{
public:
    GstRTSPServer* server = nullptr;
    GstRTSPMediaFactory* factory = nullptr;
    GstBaseSink* sink = nullptr;
    GstSample* sample = nullptr;
    GstBuffer* buffer = nullptr;
    GstMapInfo map;

    guint gstServerId = 0;
};

RtspServer::RtspServer(int& argc, char** argv, const std::string& pipeline) :
    d(new Impl)
{
    gst_init(&argc, &argv);

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
    if (d->sample) gst_sample_unref (d->sample);
    if (d->buffer) gst_buffer_unmap (d->buffer, &d->map);

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
    return reinterpret_cast< RtspServer* >(obj)->mediaConfigure(factory, media);
}

void RtspServer::mediaConfigure(GstRTSPMediaFactory*, GstRTSPMedia* media)
{
    GstElement* element = gst_rtsp_media_get_element(media);

    /* get our sink, we named it 'internal' with the name property */
    d->sink = reinterpret_cast< GstBaseSink* >(
                gst_bin_get_by_name_recurse_up (GST_BIN (element), "internal"));
    gst_object_unref(element);

    if (!d->sink)
    {
        std::cerr << "Failed to get sink with 'name=internal' property set.";
        return;
    }

    g_object_set (d->sink, "enable-last-sample", TRUE, NULL);
}

void RtspServer::spin()
{
    g_main_context_iteration(g_main_context_default(), FALSE);
}

unsigned char* RtspServer::lastFrame() const
{
    if (!d->sink) return nullptr;
    if (d->buffer) gst_buffer_unmap (d->buffer, &d->map);
    if (d->sample) gst_sample_unref (d->sample);
    g_object_get (d->sink, "last-sample", &d->sample, NULL);
    if (!d->sample) 
    {
        d->buffer = nullptr;
        return nullptr;
    }

    d->buffer = gst_sample_get_buffer (d->sample);
    gst_buffer_map (d->buffer, &d->map, GST_MAP_READ);

    return d->map.data;
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
    gst_object_unref(d->sink);
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
