#ifndef WEBRTC_STREAMER_H
#define WEBRTC_STREAMER_H

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>
#include <memory>
#include <functional>

/**
 * @brief Класс для потоковой передачи видео через WebRTC
 * 
 * Использует аппаратный энкодер H.264 на Raspberry Pi 2
 * и передает видеопоток через UDP на телефон.
 * 
 * Архитектура:
 * - Отдельный процесс для минимизации нагрузки на основное приложение
 * - Аппаратное кодирование H.264 через omxh264enc
 * - Передача через RTP/UDP для низкой задержки
 * - Поддержка камеры Raspberry Pi (PICAM) или USB-камеры
 */
class WebRtcStreamer : public QObject
{
    Q_OBJECT
public:
    explicit WebRtcStreamer(quint16 port, quint16 width, quint16 height, quint8 fps, QObject* parent = nullptr);
    virtual ~WebRtcStreamer();

    /**
     * @brief Установить адрес получателя (телефона)
     * @param host IP-адрес телефона в сети Wi-Fi
     */
    void setUdpHost(const QHostAddress& host);

    /**
     * @brief Получить порт вещания
     */
    quint16 port() const;

    /**
     * @brief Запустить потоковую передачу
     */
    void start();

    /**
     * @brief Остановить потоковую передачу
     */
    void stop();

    /**
     * @brief Установить callback для обработки кадров
     */
    void setDataCallback(const std::function<void(const void* data, int size)>& callback);

private:
    class Impl;
    Impl* d;
};

#endif // WEBRTC_STREAMER_H
