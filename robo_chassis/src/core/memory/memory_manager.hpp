#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace robo_chassis {

/**
 * @brief Структура статистики использования памяти
 */
struct MemoryStats {
    size_t total_bytes;           ///< Общая память (байт)
    size_t used_bytes;            ///< Использовано (байт)
    size_t free_bytes;            ///< Свободно (байт)
    size_t available_bytes;       ///< Доступно для приложений (байт)
    size_t buffers_bytes;         ///< Буферы ядра (байт)
    size_t cached_bytes;          ///< Кэш страниц (байт)
    
    // Детализация по процессам
    struct ProcessInfo {
        int pid;                  ///< PID процесса
        std::string name;         ///< Имя процесса
        size_t vm_size;           ///< Виртуальная память (байт)
        size_t rss_size;          ///< Резидентная память (байт)
        size_t shared_size;       ///< Разделяемая память (байт)
        float cpu_percent;        ///< Загрузка CPU (%)
    };
    
    std::vector<ProcessInfo> top_processes;  ///< Топ процессов по памяти
    
    // Вычисленные метрики
    float usage_percent() const;   ///< Процент использования
    float available_percent() const; ///< Процент доступной памяти
    
    // Проверка критического состояния
    bool is_critical(float threshold = 0.9f) const;
    
    // Форматированный вывод
    std::string to_json() const;
    std::string to_string() const;
};

/**
 * @brief Менеджер памяти для оптимизации потребления на RPi 2B
 * 
 * Особенности:
 * - Мониторинг в реальном времени
 * - Автоматическая очистка кэша при нехватке
 * - Лимиты для процессов
 * - Анализ топ процессов
 */
class MemoryManager {
public:
    static MemoryManager& instance();
    
    /**
     * @brief Инициализация менеджера
     * @param cache_clear_threshold Порог очистки кэша (0.0-1.0)
     * @param critical_threshold Критический порог (0.0-1.0)
     */
    void init(float cache_clear_threshold = 0.85f, 
              float critical_threshold = 0.95f);
    
    /**
     * @brief Получить текущую статистику
     */
    MemoryStats get_stats() const;
    
    /**
     * @brief Обновить статистику (вызывать периодически)
     */
    void update();
    
    /**
     * @brief Принудительная очистка кэша ядра
     * @return true если очистка выполнена
     */
    bool clear_kernel_cache();
    
    /**
     * @brief Проверить и выполнить автоматическую оптимизацию
     * @return true если были выполнены действия
     */
    bool auto_optimize();
    
    /**
     * @brief Получить топ процессов по потреблению памяти
     * @param count Количество процессов
     */
    std::vector<MemoryStats::ProcessInfo> get_top_processes(size_t count = 5);
    
    /**
     * @brief Установить лимит памяти для процесса
     * @param pid PID процесса
     * @param max_bytes Максимум байт (0 = без лимита)
     * @return true если успешно
     */
    bool set_process_limit(int pid, size_t max_bytes);
    
    /**
     * @brief Получить рекомендации по оптимизации
     */
    std::vector<std::string> get_recommendations() const;
    
    // Статистика за период
    size_t cache_cleared_count() const { return cache_clear_count_; }
    size_t optimization_count() const { return optimization_count_; }
    float avg_usage_percent() const;

private:
    MemoryManager() = default;
    ~MemoryManager() = default;
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    // Внутренние методы
    void parse_meminfo(MemoryStats& stats);
    bool parse_proc_status(MemoryStats::ProcessInfo& proc_info, int pid);
    bool check_and_clear_cache();
    void calculate_averages();
    
    // Конфигурация
    float cache_clear_threshold_;
    float critical_threshold_;
    
    // Состояние
    mutable MemoryStats current_stats_;
    size_t cache_clear_count_ = 0;
    size_t optimization_count_ = 0;
    std::vector<float> usage_history_;  ///< Для расчета среднего
    static constexpr size_t HISTORY_SIZE = 60;  ///< 60 последних замеров
    
    bool initialized_ = false;
};

} // namespace robo_chassis

// Макросы для удобного доступа
#define MEM_STATS robo_chassis::MemoryManager::instance().get_stats()
#define MEM_UPDATE robo_chassis::MemoryManager::instance().update()
#define MEM_OPTIMIZE robo_chassis::MemoryManager::instance().auto_optimize()
#define MEM_CLEAR_CACHE robo_chassis::MemoryManager::instance().clear_kernel_cache()
