#include "video_presenter.h"
#include "robo_model.h"
#include "settings_model.h"

#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QDebug>

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
	GstElement* sink = nullptr;
    quint16 port = 5000;

	const bool test = false; // Set "true" for test video

	int width = 0;
	int height = 0;

    QQmlApplicationEngine engine;

    void releasePipeline()
    {
    	if (pipeline)
    	{
    		gst_element_set_state (pipeline, GST_STATE_NULL);
    		sink = nullptr;
    		gst_object_unref (pipeline);
    		pipeline = nullptr;
    	}

    }
};

VideoPresenter::VideoPresenter(domain::RoboModel* model, QObject* parent) :
    QObject(parent),
    d(new Impl)
{
    d->model = model;

    gst_init (nullptr, nullptr);

#ifdef GSTREAMER_STATIC
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(libav);
    GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
    GST_PLUGIN_STATIC_REGISTER(playback);
    GST_PLUGIN_STATIC_REGISTER(udp);
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
	GST_PLUGIN_STATIC_REGISTER(autoconvert);
    GST_PLUGIN_STATIC_REGISTER(rtsp);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(qmlgl);
#endif

    // Just to register GstGLVideoItem in qml
	GstElement* sink = gst_element_factory_make ("qmlglsink", nullptr);
	gst_object_unref (GST_OBJECT (sink));

	d->engine.load (QUrl("qrc:/qml/Video.qml"));
	d->surface = static_cast< QQuickItem* >(d->engine.rootObjects().first());

    connect(d->model->settings(), &domain::SettingsModel::videoSourceChanged,
    		this, &VideoPresenter::onVideoSourceChanged);

    if (d->test)
    {
		QTimer::singleShot(1000, this, [&](){
			this->createPipeline();
			this->play();
		});
    }
}

VideoPresenter::~VideoPresenter()
{
	d->releasePipeline();

    delete d;
    gst_deinit();
}

void VideoPresenter::setUri(const QString& uri)
{
    this->stop();
	// uri now contains only video port
	if (uri.isEmpty()) return;
	qDebug() << Q_FUNC_INFO << uri;
    d->port = uri.toUInt();
}

QObject* VideoPresenter::surface() const
{
	return d->surface;
}

int VideoPresenter::width() const
{
	return d->width;
}

int VideoPresenter::height() const
{
	return d->height;
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

void VideoPresenter::onVideoSourceChanged()
{
	d->releasePipeline();

	const auto& uri = d->model->settings()->videoSource();
	if (uri.isEmpty()) return;
	this->setUri(uri);
	if (!this->createPipeline()) return;
    this->play();
}

bool VideoPresenter::createPipeline()
{
	QString pipe;
	if (d->test)
	{
		pipe = "videotestsrc name=src ! ";
	}
	else
	{
		pipe = "udpsrc name=src port=" + QString::number(d->port) + " ! "
			   "application/x-rtp,encoding-name=H264,payload=96 ! rtph264depay ! "
			   "h264parse ! avdec_h264 ! ";
	}

	pipe += "glupload ! glcolorconvert ! qmlglsink name=sink";

	GError* error = nullptr;
	d->pipeline = gst_parse_launch (pipe.toLatin1().data(), &error);

	if (error)
	{
        qWarning() << "*** GStreamer ***" << error->message;
        g_error_free(error);
        return false;
	}

	d->sink = gst_bin_get_by_name(GST_BIN(d->pipeline), "sink");
	if (!d->sink)
	{
        qWarning() << "*** GStreamer ***" << "'sink' element not found";
        return false;
	}

	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(d->pipeline));
	gst_bus_add_watch(bus, onBusMessageProxy, this);
	gst_object_unref(bus);

	g_object_set (d->sink, "widget", d->surface, nullptr);
	return true;
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
            qWarning() << "*** GStreamer ***"
            		   << gst_element_get_name(message->src)
					   << error->message;
            g_error_free(error);

			d->model->settings()->setVideoSource(QString());
			break;
        }
		case GST_MESSAGE_STATE_CHANGED:
		{
			if (message->src == GST_OBJECT (d->pipeline))
			{
				GstState oldState, newState;
				gst_message_parse_state_changed (message, &oldState,
												 &newState, nullptr);

				qDebug() << Q_FUNC_INFO
						 << QString("Element %1 changed state from %2 to %3.")
						 	 .arg(GST_OBJECT_NAME (message->src))
							 .arg(gst_element_state_get_name (oldState))
							 .arg(gst_element_state_get_name (newState));
				if (newState == GST_STATE_PLAYING)
				{
					this->updateVideoResolution();
				}
			}
		    break;
		}
        default:
            break;
    }
    return TRUE;
}

void VideoPresenter::updateVideoResolution()
{
	gint width;
	gint height;

	GstPad* pad = gst_element_get_static_pad(d->sink, "sink");
	GstCaps* caps = gst_pad_get_current_caps (pad);
	GstStructure* structure = gst_caps_get_structure(caps, 0);
	gst_caps_unref(caps);

	gst_structure_get_int(structure, "width", &width);
	gst_structure_get_int(structure, "height", &height);

	if (width != d->width || height != d->height)
	{
		d->width = width;
		d->height = height;
		emit videoSizeChanged();
	}
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
