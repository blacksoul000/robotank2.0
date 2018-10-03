#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <string>
#include <functional>

typedef struct _GstRTSPMediaFactory GstRTSPMediaFactory;
typedef struct _GstRTSPMedia GstRTSPMedia;
typedef struct _GstElement GstElement;
typedef struct _GstPad GstPad;
typedef struct _GstPadProbeInfo GstPadProbeInfo;

typedef void* gpointer;
typedef unsigned int guint;
typedef int    gint;
typedef gint   gboolean;

namespace rtsp_server
{
    class RtspServer
    {
    public:
        RtspServer(const std::string& pipeline);
        ~RtspServer();

        void start();
        void stop();

        uint16_t port() const;
        std::string streamName() const;

        static void mediaConfigureProxy(GstRTSPMediaFactory* factory, GstRTSPMedia* media,
                                        gpointer obj);

        static gboolean onTimeoutProxy(gpointer obj);
        int bufferProbe(GstPad* pad, GstPadProbeInfo* info);

        void setDataCallback(const std::function< void(const void* data, int size) >& cb);

    private:
        void mediaConfigure(GstRTSPMediaFactory* factory, GstRTSPMedia* media);
        gboolean onTimeout();

    private:
        class Impl;
        Impl* d;
    };
} // namespace rtsp_server

#endif // RTSP_SERVER_H
