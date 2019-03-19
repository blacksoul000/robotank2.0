#include "video_presenter.h"

#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>
#include <QObject>
#include <QUrl>
#include <QDebug>
#include <QRunnable>
#include <QQmlApplicationEngine>

#include <QDir>
#include <QStandardPaths>

#include <gst/gst.h>

using presentation::VideoPresenter;

static gboolean onBusMessageProxy(GstBus* bus, GstMessage* message, void* obj)
{
    return reinterpret_cast< VideoPresenter* >(obj)->onBusMessage(bus, message);
}

class VideoPresenter::Impl
{
public:
    domain::RoboModel* model = nullptr;
	GstElement* pipeline = nullptr;
	QQuickItem* surface = nullptr;
	GstElement* src = nullptr;

    QQmlApplicationEngine engine;
};

VideoPresenter::VideoPresenter(domain::RoboModel* model, QObject* parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;

    gst_init (nullptr, nullptr);

//    GST_PLUGIN_STATIC_REGISTER(coreelements);
//    GST_PLUGIN_STATIC_REGISTER(rtp);
//    GST_PLUGIN_STATIC_REGISTER(x264);
//    GST_PLUGIN_STATIC_REGISTER(openh264);
//    GST_PLUGIN_STATIC_REGISTER(libav);
//    GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
//    GST_PLUGIN_STATIC_REGISTER(playback);
//    GST_PLUGIN_STATIC_REGISTER(qt5videosink);
//    GST_PLUGIN_STATIC_REGISTER(soup);
//    GST_PLUGIN_STATIC_REGISTER(udp);
//    GST_PLUGIN_STATIC_REGISTER(rtsp);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(qmlgl);

	for (auto& p : this->pluginsList())
	{
		qDebug() << p << this->pluginFeaturesList(p);
	}

	// FIXME
	this->setUri("rtsp://192.168.1.77:8554/live");
}

VideoPresenter::~VideoPresenter()
{
    gst_element_set_state (d->pipeline, GST_STATE_NULL);
    gst_object_unref (d->pipeline);

    delete d;
    gst_deinit();
}

void VideoPresenter::setUri(const QString& uri)
{
	if (!d->pipeline)
	{
		d->src = gst_element_factory_make ("videotestsrc", nullptr);
		GstElement* glupload = gst_element_factory_make ("glupload", nullptr);
		/* the plugin must be loaded before loading the qml file to register the
		 * GstGLVideoItem qml item */
		GstElement* sink = gst_element_factory_make ("qmlglsink", nullptr);
		d->pipeline = gst_pipeline_new (nullptr);

		if (!d->src || !glupload || !sink || !d->pipeline)
		{
			qDebug() << Q_FUNC_INFO << "Not all elements could be created.";
			gst_object_unref(d->pipeline);
			d->pipeline = nullptr;
			return;
		}

		gst_bin_add_many (GST_BIN (d->pipeline), d->src, glupload, sink, nullptr);
		if (!gst_element_link_many (d->src, glupload, sink, nullptr))
		{
			qDebug() << Q_FUNC_INFO << "Elements could not be linked.";
			gst_object_unref(d->pipeline);
			d->pipeline = nullptr;
			return;
		}

		GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(d->pipeline));
		gst_bus_add_watch(bus, onBusMessageProxy, this);
		gst_object_unref(bus);

		d->engine.load (QUrl(QStringLiteral("qrc:/qml/Video.qml")));
		QQuickItem* rootObject = static_cast< QQuickItem* >(d->engine.rootObjects().first());
		d->surface = rootObject->findChild< QQuickItem* > ("videoItem");

		qDebug() << rootObject << d->surface;
		g_object_set (sink, "widget", d->surface, nullptr);
	}

//    QQmlProperty(d->player, "source").write(QUrl(uri));
}

QObject* VideoPresenter::surface() const
{
	return d->surface;
}

void VideoPresenter::play()
{
	qDebug() << Q_FUNC_INFO << d->pipeline;
	if (!d->pipeline) return;
	gst_element_set_state (d->pipeline, GST_STATE_PLAYING);
}

void VideoPresenter::stop()
{
	if (!d->pipeline) return;
	gst_element_set_state (d->pipeline, GST_STATE_PAUSED);
}

bool VideoPresenter::onBusMessage(GstBus* bus, GstMessage* message)
{
    Q_UNUSED(bus)

    switch (GST_MESSAGE_TYPE(message))
    {
        case GST_MESSAGE_ERROR:
        {
            GError* error;
            gst_message_parse_error(message, &error, nullptr);
            qWarning() << "*** GStreamer ***" << gst_element_get_name(message->src) << error->message;
            g_error_free(error);

            gst_element_set_state(d->pipeline, GST_STATE_NULL);
            gst_object_unref(d->pipeline);
            d->pipeline = nullptr;
            return FALSE;
        }
		case GST_MESSAGE_STATE_CHANGED:
		{
			GstState old_state, new_state;

			gst_message_parse_state_changed (message, &old_state, &new_state, nullptr);
			qDebug() << Q_FUNC_INFO
					 << QString("Element %1 changed state from %2 to %3.")
					 	 .arg(GST_OBJECT_NAME (message->src))
						 .arg(gst_element_state_get_name (old_state))
						 .arg(gst_element_state_get_name (new_state));
		    break;
		}
        default:
            break;
    }
    return TRUE;
}

QStringList VideoPresenter::pluginsList() const
{
	QStringList out;
	GstRegistry* registry = gst_registry_get();

	GList* l;
	for (l = gst_registry_get_plugin_list(registry); l != NULL; l = l->next)
	{
		out << gst_plugin_get_name((GstPlugin*)l->data);
	}
	gst_plugin_list_free(l);
	return out;
}

QStringList VideoPresenter::pluginFeaturesList(const QString& pluginName) const
{
	QStringList out;
	GstRegistry* registry = gst_registry_get();

	GList* l;
	for (l = gst_registry_get_feature_list_by_plugin(registry, pluginName.toUtf8().data());
		 l != NULL; l = l->next)
	{
		out << gst_plugin_feature_get_name((GstPluginFeature*)l->data);
	}
	gst_plugin_feature_list_free(l);
	return out;
}
