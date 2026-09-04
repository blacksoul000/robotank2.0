#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <functional>

// Forward declarations
struct Telemetry;
struct Command;

namespace robo_chassis {

/**
 * @brief WebSocket сервер для управления роботом через веб-интерфейс
 * 
 * Обеспечивает двустороннюю связь с браузером:
 * - Приём команд управления (джойстики, кнопки)
 * - Отправка телеметрии в реальном времени
 * - Автоматическое переподключение клиентов
 */
class WebSocketServer {
public:
    /**
     * @brief Конструктор
     * @param port Порт для прослушивания (по умолчанию 8765)
     */
    explicit WebSocketServer(int port = 8765);
    
    ~WebSocketServer();
    
    // Запрет копирования
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;
    
    /**
     * @brief Запуск сервера в отдельном потоке
     */
    void start();
    
    /**
     * @brief Остановка сервера и закрытие всех соединений
     */
    void stop();
    
    /**
     * @brief Проверка статуса работы сервера
     */
    bool isRunning() const { return m_running.load(); }
    
    /**
     * @brief Получить количество подключенных клиентов
     */
    int getClientCount() const;
    
    /**
     * @brief Отправить телеметрию всем подключенным клиентам
     * @param telemetry Данные телеметрии для отправки
     * @param cpu_temp Температура CPU (градусы Цельсия)
     * @param memory_percent Использование памяти (%)
     * @param heading Курс компаса (0-360 градусов)
     * @param mag_x Магнитометр X (мкТл)
     * @param mag_y Магнитометр Y (мкТл)
     * @param mag_z Магнитометр Z (мкТл)
     * @param ultrasonic_dist Дистанция до препятствия (см)
     * @param wifi_quality Качество WiFi сигнала (%)
     */
    void broadcastTelemetry(const Telemetry& telemetry, float cpu_temp, float memory_percent,
                           float heading = -1.0f, float mag_x = 0.0f, float mag_y = 0.0f,
                           float mag_z = 0.0f, float ultrasonic_dist = -1.0f, int wifi_quality = 0);
    
    /**
     * @brief Установить обработчик входящих команд
     * @param callback Функция обратного вызова для обработки команд
     */
    void setCommandCallback(std::function<void(const Command&)> callback);

private:
    int m_port;
    std::atomic<bool> m_running{false};
    std::thread m_server_thread;
    mutable std::mutex m_mutex;
    
    // Список подключенных клиентов (дескрипторы сокетов)
    std::vector<int> m_clients;
    
    // Обработчик команд
    std::function<void(const Command&)> m_command_callback;
    
    // Основной цикл сервера
    void serverLoop();
    
    // Обработка нового подключения
    void handleClient(int client_fd);
    
    // Чтение данных от клиента
    ssize_t readFromClient(int client_fd, unsigned char* buffer, size_t len);
    
    // Отправка данных клиенту
    ssize_t writeToClient(int client_fd, const unsigned char* data, size_t len);
    
    // Парсинг WebSocket фрейма
    bool parseWebSocketFrame(const unsigned char* buffer, size_t len, 
                            std::string& payload, bool& is_text);
    
    // Создание WebSocket фрейма
    std::vector<unsigned char> createWebSocketFrame(const std::string& payload, bool is_text = true);
    
    // HTTP handshake для WebSocket
    bool performHandshake(int client_fd);
    
    // Отправка строки клиенту
    void sendString(int client_fd, const std::string& str);
};

} // namespace robo_chassis
