#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QObject>

namespace robo
{
    class MainWindow : public QObject
    {
        Q_OBJECT

        enum Button {
            Square = 0,
            Cross = 1,
            Circle = 2,
            Triangle = 3
        };

    public:
        MainWindow();
        ~MainWindow();

    signals:
        void frameReceived();

    private slots:
        void onButtonsUpdated(quint16 buttons);

        void onImageTimeout();

//        void onNewFrame(const sensor_msgs::ImageConstPtr& msg);
        void onFrameReceived();

    private:
        class Impl;
        Impl* d;
    };
}

#endif //MAIN_WINDOW_H