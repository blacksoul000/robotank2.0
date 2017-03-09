#include "main_window.h"

//internal
#include "robo_model.h"
#include "sight_model.h"
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
    const int imageTimeout = 1000; //ms
}

class MainWindow::Impl
{
public:
    domain::RoboModel* model = nullptr;
    QQuickView* viewer = nullptr;
    ChassisExchanger* exchanger = nullptr;
    QTimer imageTimer;

    quint16 buttonsState = 0;
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

    d->imageTimer.setInterval(::imageTimeout);
    connect(&d->imageTimer, &QTimer::timeout, this, &MainWindow::onImageTimeout);

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
    connect(this, &MainWindow::frameReceived, this, &MainWindow::onFrameReceived);
}

MainWindow::~MainWindow()
{
    d->imageTimer.stop();
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

void MainWindow::onImageTimeout()
{
    d->imageTimer.stop();
    d->model->sight()->setFrame(QImage());

#ifdef ANDROID
    if (d->wakeLocked)
    {
        d->wakeLocked = false;
        d->wakeLock.callMethod<void>("release", "()V");
    }
#endif
}

//void MainWindow::onNewFrame(const sensor_msgs::ImageConstPtr& msg)
//{
////    cv::Mat frame =  cv_bridge::toCvShare(msg, "bgr8")->image;
////    d->model->sight()->setFrame(d->mat2QImage(frame));

//    emit frameReceived();
//}

void MainWindow::onFrameReceived()
{
    d->imageTimer.start();

#ifdef ANDROID
    if (!d->wakeLocked)
    {
        d->wakeLocked = true;
        d->wakeLock.callMethod<void>("acquire", "()V");
    }
#endif
}
