#include "main_window.h"

//internal
#include "robo_model.h"
#include "track_model.h"
#include "settings_model.h"
#include "status_model.h"
#include "presenter_factory.h"
#include "mavlink_exchanger.h"

//Qt
#include <QCoreApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QQmlEngine>
#include <QThread>
#include <QTimer>
#include <QDebug>

////Android
//#ifdef ANDROID
//#include <QtAndroid>
//#include <QAndroidJniObject>
//#endif

using robo::MainWindow;
using domain::MavlinkExchanger;

namespace
{
    const QString videoSourceId = "streamProtocol";
    const QString defaultVideoSourceValue = "rtsp://127.0.0.1:8554/live";
}

class MainWindow::Impl
{
public:
    QThread* thread = nullptr;
    domain::RoboModel* model = nullptr;
    QQuickView* viewer = nullptr;

    quint16 buttonsState = 0;

    template< class T >
    bool isBitSet(T value, quint8 bit) const
    {
        return (value & (1 << bit));
    }

//#ifdef ANDROID
//    QAndroidJniObject wakeLock;
//    bool wakeLocked = false;
//#endif
};

MainWindow::MainWindow() :
    QObject(),
    d(new Impl)
{
    d->thread = new QThread;
    d->model = new domain::RoboModel;
    d->model->moveToThread(d->thread);
    connect(d->thread, &QThread::started, d->model->mavlink(), &MavlinkExchanger::start);
    d->thread->start();

    d->viewer = new QQuickView;
    d->viewer->rootContext()->setContextProperty("factory",
                        new presentation::PresenterFactory(d->model, this));
    d->viewer->setSource(QUrl("qrc:/qml/Main.qml"));
    d->viewer->showFullScreen();
    d->viewer->requestActivate();

    connect(d->model->status(), &domain::StatusModel::buttonsChanged,
            this, &MainWindow::onButtonsUpdated);


//#ifdef ANDROID
//    QAndroidJniObject activity = QtAndroid::androidActivity();

//    QAndroidJniObject serviceName = QAndroidJniObject::getStaticObjectField<jstring>(
//                "android/content/Context","POWER_SERVICE");
//    QAndroidJniObject powerMgr = activity.callObjectMethod("getSystemService",
//                                            "(Ljava/lang/String;)Ljava/lang/Object;",
//                                            serviceName.object<jobject>());
//    QAndroidJniObject tag = QAndroidJniObject::fromString("Robotank");
//    d->wakeLock = powerMgr.callObjectMethod("newWakeLock",
//                                        "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;",
//                                        6, //SCREEN_DIM_WAKE_LOCK
//                                        tag.object<jstring>());
//#endif

    connect(d->viewer->engine(), &QQmlEngine::quit, qApp, &QCoreApplication::quit);
}

MainWindow::~MainWindow()
{
    d->thread->quit();
    d->thread->wait();
    delete d->thread;

    delete d->viewer;
    delete d->model;
    delete d;
}

void MainWindow::onButtonsUpdated(quint16 buttons)
{
    if (d->buttonsState == buttons) return;

    const bool squarePressed = d->isBitSet(buttons, Button::Square);
    const bool trianglePressed = d->isBitSet(buttons, Button::Triangle);

    if (squarePressed != d->isBitSet(d->buttonsState, Button::Square))
    {
        if (squarePressed)
        {
            QRectF r = d->model->track()->isTracking()
                    ? QRectF() : d->model->track()->captureRect();
            d->model->mavlink()->onTrackToggle(r);
        }
    }
    if (trianglePressed != d->isBitSet(d->buttonsState, Button::Triangle))
    {
        if (trianglePressed)
        {
            d->model->track()->nextCaptureSize();
        }
    }
    d->buttonsState = buttons;
}
