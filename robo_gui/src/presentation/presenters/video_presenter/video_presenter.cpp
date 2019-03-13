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

//static gboolean onBusMessageProxy(GstBus* bus, GstMessage* message, void* obj)
//{
//    return reinterpret_cast< VideoPresenter* >(obj)->onBusMessage(bus, message);
//}

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

    GstRegistry* registry;
    registry = gst_registry_get();
    gst_registry_scan_path(registry, QCoreApplication::applicationDirPath().toUtf8().data());
//
//    GList *l;
//
//    for (l = gst_registry_get_plugin_list(registry); l != NULL; l = l->next)
//	{
//		qDebug() << "222222222" << gst_plugin_get_name((GstPlugin*)l->data);
//	}
//    gst_plugin_list_free(l);
//
//    d->pipeline = gst_pipeline_new (nullptr);
//
//    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(d->pipeline));
//    gst_bus_add_watch(bus, onBusMessageProxy, this);
//    gst_object_unref(bus);
//
//    d->src = gst_element_factory_make ("videotestsrc", nullptr);
//    GstElement* glupload = gst_element_factory_make ("glupload", nullptr);
//    /* the plugin must be loaded before loading the qml file to register the
//     * GstGLVideoItem qml item */
//    GstElement* sink = gst_element_factory_make ("qmlglsink", nullptr);
//
//    gst_bin_add_many (GST_BIN (d->pipeline), d->src, glupload, sink, nullptr);
//    gst_element_link_many (d->src, glupload, sink, nullptr);
//
//    d->engine.load (QUrl(QStringLiteral("qrc:/qml/Video.qml")));
//    QQuickItem* rootObject = static_cast< QQuickItem*>(d->engine.rootObjects().first());
//    d->surface = rootObject->findChild< QQuickItem* > ("videoItem");
//    g_object_set (sink, "widget", d->surface, nullptr);
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
//    QQmlProperty(d->player, "source").write(QUrl(uri));
}

QObject* VideoPresenter::surface() const
{
	return d->surface;
}

void VideoPresenter::play()
{
	if (!d->pipeline) return;
	gst_element_set_state (d->pipeline, GST_STATE_PLAYING);
}

void VideoPresenter::stop()
{
	if (!d->pipeline) return;
	gst_element_set_state (d->pipeline, GST_STATE_PAUSED);
}

//bool VideoPresenter::onBusMessage(GstBus* bus, GstMessage* message)
//{
//    Q_UNUSED(bus)

//    switch (GST_MESSAGE_TYPE(message))
//    {
//        case GST_MESSAGE_ERROR:
//            GError* error;
//            gst_message_parse_error(message, &error, nullptr);
//            qFatal() << "*** GStreamer ***" << gst_element_get_name(message->src) << error->message;
//            g_error_free(error);

//            gst_element_set_state(d->pipeline, GST_STATE_NULL);
//            gst_object_unref(d->pipeline);
//            d->pipeline = nullptr;
//            return FALSE;

//        default:
//            break;
//    }
//    return TRUE;
//}
