/**
 * @brief main.cpp - Точка входа для отдельного процесса WebRTC-стриминга
 * 
 * Этот процесс запускается независимо от основного приложения robo_chassis
 * и отвечает только за передачу видеопотока на телефон через Wi-Fi.
 * 
 * Использование:
 *   ./webrtc_streamer [--port PORT] [--width WIDTH] [--height HEIGHT] [--fps FPS] [--host IP]
 * 
 * Пример:
 *   ./webrtc_streamer --port 8554 --width 640 --height 480 --fps 30 --host 192.168.1.100
 * 
 * Аргументы:
 *   --port    Порт UDP для вещания (по умолчанию 8554)
 *   --width   Ширина кадра (по умолчанию 640)
 *   --height  Высота кадра (по умолчанию 480)
 *   --fps     Частота кадров (по умолчанию 30)
 *   --host    IP-адрес телефона в сети Wi-Fi (обязательно)
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QHostAddress>
#include <QDebug>
#include <csignal>
#include <memory>

#include "webrtc_streamer.h"

namespace {
    std::unique_ptr<WebRtcStreamer> g_streamer;
    
    void signalHandler(int signum)
    {
        qDebug() << "Received signal" << signum << ", shutting down...";
        if (g_streamer)
        {
            g_streamer->stop();
            g_streamer.reset();
        }
        QCoreApplication::quit();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("WebRTC Streamer");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("WebRTC video streamer for Raspberry Pi 2 with hardware H.264 encoding");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  "UDP port for streaming (default: 8554)",
                                  "port", "8554");
    parser.addOption(portOption);

    QCommandLineOption widthOption(QStringList() << "w" << "width",
                                   "Video width (default: 640)",
                                   "width", "640");
    parser.addOption(widthOption);

    QCommandLineOption heightOption(QStringList() << "h" << "height",
                                    "Video height (default: 480)",
                                    "height", "480");
    parser.addOption(heightOption);

    QCommandLineOption fpsOption(QStringList() << "f" << "fps",
                                 "Frames per second (default: 30)",
                                 "fps", "30");
    parser.addOption(fpsOption);

    QCommandLineOption hostOption(QStringList() << "H" << "host",
                                  "IP address of the phone (required)",
                                  "host");
    parser.addOption(hostOption);

    parser.process(app);

    // Проверяем обязательный параметр --host
    if (!parser.isSet(hostOption))
    {
        qCritical() << "Error: --host option is required!";
        qCritical() << "Usage: webrtc_streamer --host <phone_ip> [options]";
        return 1;
    }

    quint16 port = parser.value(portOption).toUShort();
    quint16 width = parser.value(widthOption).toUShort();
    quint16 height = parser.value(heightOption).toUShort();
    quint8 fps = parser.value(fpsOption).toUShort();
    QHostAddress host(parser.value(hostOption));

    if (host.isNull())
    {
        qCritical() << "Error: Invalid host address:" << parser.value(hostOption);
        return 1;
    }

    qDebug() << "Starting WebRTC Streamer...";
    qDebug() << "  Resolution:" << width << "x" << height;
    qDebug() << "  FPS:" << (int)fps;
    qDebug() << "  Port:" << port;
    qDebug() << "  Host:" << host.toString();

    // Регистрируем обработчики сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Создаем и запускаем стример
    g_streamer = std::make_unique<WebRtcStreamer>(port, width, height, fps);
    g_streamer->setUdpHost(host);
    g_streamer->start();

    qDebug() << "WebRTC Streamer is running. Press Ctrl+C to stop.";

    return app.exec();
}
