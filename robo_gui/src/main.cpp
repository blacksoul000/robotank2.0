#include "main_window.h"

#include <QGuiApplication>

int main(int argc, char** argv)
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    robo::MainWindow mw;
    return app.exec();
}
