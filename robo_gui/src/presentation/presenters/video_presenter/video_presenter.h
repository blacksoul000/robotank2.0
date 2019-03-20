#ifndef VIDEO_PRESENTER_H
#define VIDEO_PRESENTER_H

#include <QObject>

#include <gst/gst.h>

extern "C" {
	GST_PLUGIN_STATIC_DECLARE(coreelements);
	GST_PLUGIN_STATIC_DECLARE(rtpmanager);
	GST_PLUGIN_STATIC_DECLARE(rtp);
	GST_PLUGIN_STATIC_DECLARE(libav);
	GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
	GST_PLUGIN_STATIC_DECLARE(playback);
	GST_PLUGIN_STATIC_DECLARE(udp);
	GST_PLUGIN_STATIC_DECLARE(videoconvert);
	GST_PLUGIN_STATIC_DECLARE(autoconvert);
	GST_PLUGIN_STATIC_DECLARE(rtsp);
	GST_PLUGIN_STATIC_DECLARE(videotestsrc);
	GST_PLUGIN_STATIC_DECLARE(opengl);
	GST_PLUGIN_STATIC_DECLARE(qmlgl);
}

namespace domain
{
    class RoboModel;
} // namespace domain

namespace presentation
{
    class VideoPresenter : public QObject
    {
        Q_OBJECT
		Q_PROPERTY(QObject* surface READ surface CONSTANT)

    public:
		VideoPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~VideoPresenter();

        QObject* surface() const;

    	QStringList pluginsList() const;
    	QStringList pluginFeaturesList(const QString& pluginName) const;

	public slots:
        void play();
        void stop();
        void setUri(const QString & uri);
        bool onBusMessage(GstBus* bus, GstMessage* message);

    private:
        class Impl;
        Impl* d;
    };
} // namespace presentation

#endif //VIDEO_PRESENTER_H
