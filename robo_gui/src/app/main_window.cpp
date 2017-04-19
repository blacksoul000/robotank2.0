#include "main_window.h"

//internal
#include "robo_model.h"
#include "track_model.h"
#include "settings_model.h"
#include "status_model.h"
#include "presenter_factory.h"
#include "chassis_exchanger.h"

//Qt
#include <QCoreApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QQmlEngine>
#include <QSettings>
#include <QTimer>
#include <QDebug>

//Android
#ifdef ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#endif

using robo::MainWindow;
using domain::ChassisExchanger;

namespace
{
    const QString protocolId = "streamProtocol";
    const QString hostId = "streamHost";
    const QString portId = "streamPort";
    const QString streamNameId = "streamName";

    const QString defaultProtocolValue = "rtsp";
    const QString defaultHostValue = "127.0.0.1";
    const QString defaultPortValue = "8554";
    const QString defaultStreamNameValue = "live";
}

class MainWindow::Impl
{
public:
    domain::RoboModel* model = nullptr;
    QQuickView* viewer = nullptr;
    ChassisExchanger* exchanger = nullptr;

    quint16 buttonsState = 0;

#ifdef ANDROID
    QAndroidJniObject wakeLock;
    bool wakeLocked = false;
#endif
};

MainWindow::MainWindow() :
    QObject(),
    d(new Impl)
{
    d->model = new domain::RoboModel;
    d->exchanger = new ChassisExchanger(d->model, this);
    d->viewer = new QQuickView;
    d->viewer->rootContext()->setContextProperty("factory",
                        new presentation::PresenterFactory(d->model, this));
    d->viewer->setSource(QUrl("qrc:/qml/Main.qml"));
    d->viewer->showFullScreen();
    d->viewer->requestActivate();

    connect(d->exchanger, &ChassisExchanger::buttonsUpdated, this, &MainWindow::onButtonsUpdated);


#ifdef ANDROID
    QAndroidJniObject activity = QtAndroid::androidActivity();

    QAndroidJniObject serviceName = QAndroidJniObject::getStaticObjectField<jstring>(
                "android/content/Context","POWER_SERVICE");
    QAndroidJniObject powerMgr = activity.callObjectMethod("getSystemService",
                                            "(Ljava/lang/String;)Ljava/lang/Object;",
                                            serviceName.object<jobject>());
    QAndroidJniObject tag = QAndroidJniObject::fromString("Robotank");
    d->wakeLock = powerMgr.callObjectMethod("newWakeLock",
                                        "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;",
                                        6, //SCREEN_DIM_WAKE_LOCK
                                        tag.object<jstring>());
#endif

    connect(d->viewer->engine(), &QQmlEngine::quit, qApp, &QCoreApplication::quit);

    this->loadSettings();
    connect(d->model->settings(), &domain::SettingsModel::streamProtocolChanged,
            this, &MainWindow::saveSettings);
    connect(d->model->settings(), &domain::SettingsModel::streamHostChanged,
            this, &MainWindow::saveSettings);
    connect(d->model->settings(), &domain::SettingsModel::streamPortChanged,
            this, &MainWindow::saveSettings);
    connect(d->model->settings(), &domain::SettingsModel::streamNameChanged,
            this, &MainWindow::saveSettings);
}

MainWindow::~MainWindow()
{
    delete d->viewer;
    delete d->model;
    delete d;
}

void MainWindow::onButtonsUpdated(quint16 buttons)
{
    if (d->buttonsState == buttons) return;
    if ((buttons | Button::Square) != (d->buttonsState | Button::Square))
    {
        if ((buttons | Button::Square) != Button::Square) // Button is released
        {
            QRectF r = d->model->track()->isTracking()
                    ? QRectF() : d->model->track()->captureRect();
            d->exchanger->onTrackToggle(r);
        }
    }
    if ((buttons | Button::Triangle) != (d->buttonsState | Button::Triangle))
    {
        if ((buttons | Button::Triangle) != Button::Triangle) // Button is released
        {
            d->model->track()->nextCaptureSize();
        }
    }
    d->buttonsState = buttons;
}

void MainWindow::loadSettings()
{
    QSettings settings;
    d->model->settings()->setStreamProtocol(
                settings.value(::protocolId, ::defaultProtocolValue).toString());
    d->model->settings()->setStreamHost(
                settings.value(::hostId, ::defaultHostValue).toString());
    d->model->settings()->setStreamPort(
                settings.value(::portId, ::defaultPortValue).toString());
    d->model->settings()->setStreamName(
                settings.value(::streamNameId, ::defaultStreamNameValue).toString());
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue(::protocolId, d->model->settings()->streamProtocol());
    settings.setValue(::hostId, d->model->settings()->streamHost());
    settings.setValue(::portId, d->model->settings()->streamPort());
    settings.setValue(::streamNameId, d->model->settings()->streamName());
}
