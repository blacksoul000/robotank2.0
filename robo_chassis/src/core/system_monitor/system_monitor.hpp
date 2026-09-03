#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <vector>

namespace robo_chassis {

/**
 * @brief Структура с данными о состоянии системы
 */
struct SystemStats {
    // CPU
    float cpu_usage_percent;           // Загрузка CPU в % (0-100)
    float cpu_temperature_celsius;     // Температура CPU в °C
    uint32_t cpu_frequency_mhz;        // Частота CPU в MHz
    
    // Память
    uint64_t memory_total_bytes;       // Всего памяти
    uint64_t memory_used_bytes;        // Использовано памяти
    uint64_t memory_free_bytes;        // Свободно памяти
    float memory_usage_percent;        // Процент использования памяти
    
    // Диск
    uint64_t disk_total_bytes;         // Всего места на диске
    uint64_t disk_used_bytes;          // Использовано места
    uint64_t disk_free_bytes;          // Свободно места
    float disk_usage_percent;          // Процент использования диска
    
    // Сеть
    uint64_t network_rx_bytes;         // Получено байт
    uint64_t network_tx_bytes;         // Отправлено байт
    int wifi_rssi;                     // RSSI WiFi сигнала (dBm, обычно -90..-30)
    int wifi_link_quality;             // Качество соединения (0-100%)
    
    // Питание
    float battery_voltage;             // Напряжение батареи (V)
    bool battery_low;                  // Флаг низкого заряда
    
    // Периферия
    bool arduino_connected;            // Arduino подключена
    bool imu_ready;                    // IMU готов
    bool compass_ready;                // Компас готов
    bool ultrasonic_ready;             // Ультразвук готов
    float distance_cm;                 // Расстояние до препятствия (см)
    float compass_heading;             // Курс от компаса (градусы 0-360)
    
    // Троттлинг (RPi специфично)
    bool is_throttled;                 // Флаг троттлинга
    bool is_under_voltage;             // Пониженное напряжение
    bool is_freq_capped;               // Частота ограничена
    bool is_overheated;                // Перегрев
    
    // Время последнего обновления
    uint64_t timestamp_ms;             // Timestamp в миллисекундах
    
    SystemStats() 
        : cpu_usage_percent(0.0f)
        , cpu_temperature_celsius(0.0f)
        , cpu_frequency_mhz(0)
        , memory_total_bytes(0)
        , memory_used_bytes(0)
        , memory_free_bytes(0)
        , memory_usage_percent(0.0f)
        , disk_total_bytes(0)
        , disk_used_bytes(0)
        , disk_free_bytes(0)
        , disk_usage_percent(0.0f)
        , network_rx_bytes(0)
        , network_tx_bytes(0)
        , wifi_rssi(0)
        , wifi_link_quality(0)
        , battery_voltage(0.0f)
        , battery_low(false)
        , arduino_connected(false)
        , imu_ready(false)
        , compass_ready(false)
        , ultrasonic_ready(false)
        , distance_cm(0.0f)
        , compass_heading(0.0f)
        , is_throttled(false)
        , is_under_voltage(false)
        , is_freq_capped(false)
        , is_overheated(false)
        , timestamp_ms(0) {}
};

/**
 * @brief Пороговые значения для троттлинга и предупреждений
 */
struct ThrottleThresholds {
    float temperature_warning;      // Предупреждение о температуре (°C)
    float temperature_critical;     // Критическая температура (°C)
    float memory_warning;           // Предупреждение о памяти (%)
    float memory_critical;          // Критический уровень памяти (%)
    float cpu_warning;              // Предупреждение о загрузке CPU (%)
    float cpu_critical;             // Критическая загрузка CPU (%)
    
    ThrottleThresholds()
        : temperature_warning(70.0f)
        , temperature_critical(80.0f)
        , memory_warning(80.0f)
        , memory_critical(95.0f)
        , cpu_warning(80.0f)
        , cpu_critical(95.0f) {}
};

/**
 * @brief Класс для мониторинга системных ресурсов Raspberry Pi
 * 
 * Предоставляет информацию о:
 * - Загрузке CPU
 * - Температуре CPU
 * - Использовании памяти
 * - Использовании диска
 * - Сетевой активности
 * - Статусе троттлинга (RPi специфично)
 */
class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();
    
    /**
     * @brief Обновить все метрики
     * @return true если успешно
     */
    bool update();
    
    /**
     * @brief Получить текущую статистику
     */
    const SystemStats& getStats() const { return stats_; }
    
    /**
     * @brief Получить температуру CPU
     */
    float getCpuTemperature() const { return stats_.cpu_temperature_celsius; }
    
    /**
     * @brief Получить загрузку CPU
     */
    float getCpuUsage() const { return stats_.cpu_usage_percent; }
    
    /**
     * @brief Получить процент использования памяти
     */
    float getMemoryUsagePercent() const { return stats_.memory_usage_percent; }
    
    /**
     * @brief Проверить, требуется ли троттлинг
     */
    bool needsThrottling() const;
    
    /**
     * @brief Получить рекомендации по оптимизации
     * @return Вектор строк с рекомендациями
     */
    std::vector<std::string> getRecommendations() const;
    
    /**
     * @brief Сбросить сетевую статистику (для измерения дельты)
     */
    void resetNetworkStats();
    
    /**
     * @brief Получить JSON представление статистики
     */
    std::string toJson() const;
    
private:
    SystemStats stats_;
    ThrottleThresholds thresholds_;
    
    // Для расчета CPU usage
    uint64_t prev_idle_time_ = 0;
    uint64_t prev_total_time_ = 0;
    
    // Для расчета сети
    uint64_t prev_rx_bytes_ = 0;
    uint64_t prev_tx_bytes_ = 0;
    
    // Методы чтения
    float readCpuTemperature() const;
    float readCpuUsage();
    void readMemoryStats();
    void readDiskStats();
    void readNetworkStats();
    void readThrottlingStatus();
    uint32_t readCpuFrequency() const;
    
    // Хелперы для парсинга /proc
    static std::vector<std::string> readFileLines(const std::string& path);
    static std::optional<uint64_t> parseMemInfo(const std::string& label);
};

} // namespace robo_chassis
