#include "rtsp_server.h"

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>

using rtsp_server::RtspServer;

class RtspServer::Impl
{
public:
    GMainLoop* loop = nullptr;
    GstRTSPServer* server = nullptr;
    GstRTSPMediaFactory* factory = nullptr;
    GstAppSrc* appsrc = nullptr;
    GstClockTime timestamp = 0;
    guint gstServerId = 0;

    int fps = 0;
    int width = 640;
    int height = 480;
    std::string format;
};

RtspServer::RtspServer(int& argc, char** argv,
                       const std::string& pipeline, int width, int height, int fps,
                       const std::string& format) :
    d(new Impl)
{
    d->width = width;
    d->height = height;
    d->format = format;
    d->fps = fps;

    gst_init(&argc, &argv);

    /* create a server instance */
    d->server = gst_rtsp_server_new();

    /* make a media factory for a test stream. The default media factory can use
     * gst-launch syntax to create pipelines.
     * any launch line works as long as it contains elements named pay%d. Each
     * element with pay%d names will be a stream */
    d->factory = gst_rtsp_media_factory_new();

    gst_rtsp_media_factory_set_shared (d->factory, TRUE);

    /* notify when our media is ready, This is called whenever someone asks for
     * the media and a new pipeline with our appsrc is created */
    g_signal_connect(d->factory, "media-configure",
                     G_CALLBACK(&RtspServer::mediaConfigureProxy), this);

    gst_rtsp_media_factory_set_launch(
                d->factory,
                std::string("( appsrc name=internal_src is-live=true do-timestamp=true ! "
                            + pipeline + " )").c_str());
}

RtspServer::~RtspServer()
{
    this->stop();

    gst_object_unref(d->factory);
    gst_object_unref(d->server);
    delete d;
}

void RtspServer::setHost(const std::string& host)
{
    gst_rtsp_server_set_address(d->server, host.c_str());
}

void RtspServer::setPort(uint16_t port)
{
    gst_rtsp_server_set_service(d->server, std::to_string(port).c_str());
}

void RtspServer::setStreamName(const std::string& name)
{
    /* get the mount points for this server, every server has a default object
     * that be used to map uri mount points to media factories */
    GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(d->server);
    gst_rtsp_mount_points_add_factory(mounts, std::string("/" + name).c_str(), d->factory);
    g_object_unref(mounts);
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

    /* get our appsrc, we named it 'internal_src' with the name property */
    d->appsrc = (GstAppSrc*)gst_bin_get_by_name_recurse_up(GST_BIN (element), "internal_src");

    gst_object_unref(element);

    g_object_set (G_OBJECT (d->appsrc), "caps",
                  gst_caps_new_simple ("video/x-raw",
                                       "format", G_TYPE_STRING, d->format.c_str(),
                                       "width", G_TYPE_INT, d->width,
                                       "height", G_TYPE_INT, d->height,
                                       "framerate", GST_TYPE_FRACTION, 0, 1,
                                       NULL), NULL);

    /* setup appsrc */
    g_object_set (G_OBJECT (d->appsrc),
        "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
        "format", GST_FORMAT_TIME,
        "is-live", TRUE,
        NULL);

    d->timestamp = 0;
}

bool RtspServer::write(const char* data, int size)
{
    g_main_context_iteration(g_main_context_default(), FALSE);
    if (!d->appsrc) return true;

    GstBuffer* buffer = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
                                                     (gpointer)data, size, 0, size,
                                                     NULL, NULL);

    GST_BUFFER_PTS (buffer) = d->timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, d->fps);

    d->timestamp += GST_BUFFER_DURATION (buffer);
    GstFlowReturn ret;
    g_signal_emit_by_name (d->appsrc, "push-buffer", buffer, &ret);
    g_main_context_iteration(g_main_context_default(), FALSE);

    return (ret == GST_FLOW_OK);
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
    if (d->appsrc) gst_object_unref(d->appsrc);
    g_source_remove(d->gstServerId);
}
