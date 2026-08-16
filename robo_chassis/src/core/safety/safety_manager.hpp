#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <csignal>

namespace robo_chassis {

/**
 * @brief Менеджер безопасности для мониторинга watchdog и graceful shutdown
 * 
 * Функции:
 * - Аппаратный watchdog через /dev/watchdog
 * - Программный watchdog для потери связи с Arduino
 * - Graceful shutdown при потере WebSocket/TCP клиентов
 * - Автоматическая остановка двигателей при критических сбоях
 */
class SafetyManager {
public:
    /**
     * @brief Конструктор
     * @param watchdog_timeout_sec Таймаут watchdog в секундах (перезагрузка системы)
     * @param arduino_timeout_sec Таймаут потери связи с Arduino (безопасный режим)
     */
    explicit SafetyManager(int watchdog_timeout_sec = 10, int arduino_timeout_sec = 5);
    
    ~SafetyManager();
    
    // Запрет копирования
    SafetyManager(const SafetyManager&) = delete;
    SafetyManager& operator=(const SafetyManager&) = delete;
    
    /**
     * @brief Инициализация менеджера безопасности
     * @return true если успешно инициализирован
     */
    bool init();
    
    /**
     * @brief Запуск watchdog потока
     */
    void start();
    
    /**
     * @brief Остановка всех потоков и закрытие watchdog
     */
    void stop();
    
    /**
     * @brief Сброс счётчика watchdog (вызывать периодически в основном цикле)
     */
    void petWatchdog();
    
    /**
     * @brief Обновление статуса связи с Arduino
     * @param online true если Arduino онлайн
     */
    void updateArduinoStatus(bool online);
    
    /**
     * @brief Проверка активности безопасного режима
     * @return true если активен безопасный режим (двигатели остановлены)
     */
    bool isSafeModeActive() const { return m_safe_mode_active.load(); }
    
    /**
     * @brief Проверка необходимости перезагрузки системы
     * @return true если требуется перезагрузка
     */
    bool needsReboot() const { return m_needs_reboot.load(); }
    
    /**
     * @brief Установка обработчика для аварийной остановки
     * @param callback Функция обратного вызова для остановки двигателей
     */
    void setEmergencyStopCallback(std::function<void()> callback);
    
    /**
     * @brief Принудительная активация безопасного режима
     */
    void activateSafeMode();
    
    /**
     * @brief Деактивация безопасного режима
     */
    void deactivateSafeMode();

private:
    /**
     * @brief Поток watchdog для мониторинга системы
     */
    void watchdogThread();
    
    /**
     * @brief Обработчик сигналов UNIX
     * @param signum Номер сигнала
     */
    static void signalHandler(int signum);
    
    /**
     * @brief Выполнение graceful shutdown
     */
    void gracefulShutdown();
    
    /**
     * @brief Открытие устройства /dev/watchdog
     * @return true если успешно открыто
     */
    bool openHardwareWatchdog();
    
    /**
     * @brief Закрытие устройства /dev/watchdog
     */
    void closeHardwareWatchdog();
    
    int m_watchdog_timeout_sec;
    int m_arduino_timeout_sec;
    
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_safe_mode_active{false};
    std::atomic<bool> m_needs_reboot{false};
    std::atomic<int> m_watchdog_counter{0};
    std::atomic<int> m_arduino_loss_counter{0};
    
    std::thread m_watchdog_thread;
    int m_watchdog_fd{-1};  // Файловый дескриптор /dev/watchdog
    
    std::chrono::steady_clock::time_point m_last_pet_time;
    std::chrono::steady_clock::time_point m_last_arduino_contact;
    
    std::function<void()> m_emergency_stop_callback;
    
    static SafetyManager* s_instance;  // Singleton для обработчика сигналов
};

} // namespace robo_chassis
