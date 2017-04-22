#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <string>

typedef struct _GstRTSPMediaFactory GstRTSPMediaFactory;
typedef struct _GstRTSPMedia GstRTSPMedia;
typedef struct _GstElement GstElement;
typedef void* gpointer;
typedef unsigned int guint;
typedef int    gint;
typedef gint   gboolean;

namespace rtsp_server
{
    class RtspServer
    {
    public:
        RtspServer(int& argc, char** argv,
                   const std::string& pipeline, int width, int height, int fps,
                   const std::string& format);
        ~RtspServer();

        void setHost(const std::string& host);
        void setPort(uint16_t port);
        void setStreamName(const std::string& name);

        void start();
        void stop();
        bool write(const char* data, int size);

        static void mediaConfigureProxy(GstRTSPMediaFactory* factory, GstRTSPMedia* media,
                                        gpointer obj);

        static gboolean onTimeoutProxy(gpointer obj);

    private:
        void mediaConfigure(GstRTSPMediaFactory* factory, GstRTSPMedia* media);
        gboolean onTimeout();

    private:
        class Impl;
        Impl* d;
    };
} // namespace rtsp_server

#endif // RTSP_SERVER_H
