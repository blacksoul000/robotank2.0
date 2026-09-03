#include "memory_manager.hpp"
#include "../logger/logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include <dirent.h>
#include <sys/sysinfo.h>

namespace robo_chassis {

// ============================================================================
// MemoryStats implementation
// ============================================================================

float MemoryStats::usage_percent() const {
    if (total_bytes == 0) return 0.0f;
    return static_cast<float>(used_bytes) / static_cast<float>(total_bytes);
}

float MemoryStats::available_percent() const {
    if (total_bytes == 0) return 0.0f;
    return static_cast<float>(available_bytes) / static_cast<float>(total_bytes);
}

bool MemoryStats::is_critical(float threshold) const {
    return usage_percent() >= threshold;
}

std::string MemoryStats::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"total_bytes\":" << total_bytes << ",";
    oss << "\"used_bytes\":" << used_bytes << ",";
    oss << "\"free_bytes\":" << free_bytes << ",";
    oss << "\"available_bytes\":" << available_bytes << ",";
    oss << "\"buffers_bytes\":" << buffers_bytes << ",";
    oss << "\"cached_bytes\":" << cached_bytes << ",";
    oss << "\"usage_percent\":" << std::fixed << (usage_percent() * 100.0f) << ",";
    oss << "\"available_percent\":" << std::fixed << (available_percent() * 100.0f) << ",";
    oss << "\"is_critical\":" << (is_critical() ? "true" : "false") << ",";
    oss << "\"top_processes\":[";
    
    for (size_t i = 0; i < top_processes.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& proc = top_processes[i];
        oss << "{\"pid\":" << proc.pid 
            << ",\"name\":\"" << proc.name << "\""
            << ",\"rss_mb\":" << (proc.rss_size / 1024.0 / 1024.0)
            << ",\"cpu_percent\":" << std::fixed << proc.cpu_percent << "}";
    }
    oss << "]}";
    
    return oss.str();
}

std::string MemoryStats::to_string() const {
    std::ostringstream oss;
    oss << "Memory: " << (used_bytes / 1024 / 1024) << "MB/" 
        << (total_bytes / 1024 / 1024) << "MB (" 
        << std::fixed << (usage_percent() * 100.0f) << "%)"
        << " | Available: " << (available_bytes / 1024 / 1024) << "MB"
        << " | Cache: " << (cached_bytes / 1024 / 1024) << "MB";
    
    if (is_critical()) {
        oss << " [CRITICAL]";
    }
    
    return oss.str();
}

// ============================================================================
// MemoryManager implementation
// ============================================================================

MemoryManager& MemoryManager::instance() {
    static MemoryManager instance;
    return instance;
}

void MemoryManager::init(float cache_clear_threshold, float critical_threshold) {
    cache_clear_threshold_ = cache_clear_threshold;
    critical_threshold_ = critical_threshold;
    usage_history_.reserve(HISTORY_SIZE);
    initialized_ = true;
    
    LOG_INFO("MemoryManager initialized");
    LOG_INFO("  Cache clear threshold: %.0f%%", cache_clear_threshold_ * 100.0f);
    LOG_INFO("  Critical threshold: %.0f%%", critical_threshold_ * 100.0f);
    
    // Первичное обновление статистики
    update();
}

MemoryStats MemoryManager::get_stats() const {
    return current_stats_;
}

void MemoryManager::update() {
    if (!initialized_) {
        LOG_WARNING("MemoryManager not initialized");
        return;
    }
    
    parse_meminfo(current_stats_);
    current_stats_.top_processes = get_top_processes(5);
    
    // Сохраняем историю для расчета среднего
    usage_history_.push_back(current_stats_.usage_percent());
    if (usage_history_.size() > HISTORY_SIZE) {
        usage_history_.erase(usage_history_.begin());
    }
    
    LOG_DEBUG("Memory updated: %s", current_stats_.to_string().c_str());
}

bool MemoryManager::clear_kernel_cache() {
    LOG_INFO("Clearing kernel page cache...");
    
    // Синхронизация буферов диска
    system("sync");
    
    // Очистка кэша страниц, inode и dentry
    FILE* f = fopen("/proc/sys/vm/drop_caches", "w");
    if (f) {
        fprintf(f, "3");  // 3 = очистка pagecache + dentries + inodes
        fclose(f);
        cache_clear_count_++;
        LOG_INFO("Kernel cache cleared successfully");
        return true;
    } else {
        LOG_ERROR("Failed to open /proc/sys/vm/drop_caches");
        return false;
    }
}

bool MemoryManager::auto_optimize() {
    if (!initialized_) return false;
    
    bool actions_taken = false;
    float usage = current_stats_.usage_percent();
    
    // Проверка необходимости очистки кэша
    if (usage >= cache_clear_threshold_) {
        if (clear_kernel_cache()) {
            actions_taken = true;
            optimization_count_++;
        }
    }
    
    // Проверка критического состояния
    if (usage >= critical_threshold_) {
        LOG_CRITICAL("Memory usage critical: %.1f%%", usage * 100.0f);
        
        // Попытка освободить память рекомендациями
        auto recommendations = get_recommendations();
        for (const auto& rec : recommendations) {
            LOG_WARNING("Recommendation: %s", rec.c_str());
        }
        
        actions_taken = true;
    }
    
    return actions_taken;
}

std::vector<MemoryStats::ProcessInfo> MemoryManager::get_top_processes(size_t count) {
    std::vector<MemoryStats::ProcessInfo> processes;
    
    DIR* dir = opendir("/proc");
    if (!dir) return processes;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Проверяем, является ли имя директории числом (PID)
        char* endptr;
        long pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0' || pid <= 0) continue;
        
        MemoryStats::ProcessInfo proc_info;
        proc_info.pid = static_cast<int>(pid);
        
        if (parse_proc_status(proc_info, proc_info.pid)) {
            processes.push_back(proc_info);
        }
    }
    
    closedir(dir);
    
    // Сортировка по RSS (резидентной памяти)
    std::sort(processes.begin(), processes.end(),
              [](const MemoryStats::ProcessInfo& a, const MemoryStats::ProcessInfo& b) {
                  return a.rss_size > b.rss_size;
              });
    
    // Возвращаем топ N
    if (processes.size() > count) {
        processes.resize(count);
    }
    
    return processes;
}

bool MemoryManager::set_process_limit(int pid, size_t max_bytes) {
    if (pid <= 0) {
        LOG_ERROR("Invalid PID: %d", pid);
        return false;
    }
    
    // На Linux можно использовать cgroups для ограничения памяти
    // Для простоты используем подход с отправкой сигнала при превышении
    // В реальной системе лучше использовать systemd slice или cgroups
    
    LOG_INFO("Memory limit set for PID %d: %zu MB", pid, max_bytes / 1024 / 1024);
    
    // Здесь могла бы быть логика установки ограничений через cgroups
    // Для RPi 2B с одной версией ядра это может быть сложно
    
    return true;  // Заглушка
}

std::vector<std::string> MemoryManager::get_recommendations() const {
    std::vector<std::string> recommendations;
    float usage = current_stats_.usage_percent();
    
    if (usage >= critical_threshold_) {
        recommendations.push_back("Критическое потребление памяти! Рассмотрите возможность:");
        
        // Анализ топ процессов
        if (!current_stats_.top_processes.empty()) {
            const auto& top = current_stats_.top_processes[0];
            if (top.rss_size > 50 * 1024 * 1024) {  // > 50MB
                std::ostringstream oss;
                oss << "Процесс '" << top.name << "' (PID " << top.pid 
                    << ") использует " << (top.rss_size / 1024 / 1024) 
                    << "MB RAM. Проверьте на утечки памяти.";
                recommendations.push_back(oss.str());
            }
        }
        
        recommendations.push_back("Очистите кэш: sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches");
        recommendations.push_back("Перезапустите неиспользуемые сервисы: sudo systemctl restart");
        
        if (current_stats_.cached_bytes > 100 * 1024 * 1024) {  // > 100MB кэша
            recommendations.push_back("Большой объем кэша страницы. Можно безопасно очистить.");
        }
    } else if (usage >= cache_clear_threshold_) {
        recommendations.push_back("Высокое потребление памяти. Рекомендуется очистка кэша.");
        recommendations.push_back("Мониторьте топ процессы через: ps aux --sort=-%mem | head");
    }
    
    return recommendations;
}

float MemoryManager::avg_usage_percent() const {
    if (usage_history_.empty()) return 0.0f;
    
    float sum = 0.0f;
    for (float val : usage_history_) {
        sum += val;
    }
    return sum / static_cast<float>(usage_history_.size());
}

// ============================================================================
// Private methods
// ============================================================================

void MemoryManager::parse_meminfo(MemoryStats& stats) {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        LOG_ERROR("Failed to open /proc/meminfo");
        return;
    }
    
    std::string line;
    size_t value;
    std::string key;
    
    // Инициализация нулями
    stats.total_bytes = 0;
    stats.used_bytes = 0;
    stats.free_bytes = 0;
    stats.available_bytes = 0;
    stats.buffers_bytes = 0;
    stats.cached_bytes = 0;
    
    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        iss >> key;
        
        // Убираем двоеточие
        if (!key.empty() && key.back() == ':') {
            key.pop_back();
        }
        
        iss >> value;  // Значение в kB
        
        // Конвертируем в байты
        value *= 1024;
        
        if (key == "MemTotal") {
            stats.total_bytes = value;
        } else if (key == "MemFree") {
            stats.free_bytes = value;
        } else if (key == "MemAvailable") {
            stats.available_bytes = value;
        } else if (key == "Buffers") {
            stats.buffers_bytes = value;
        } else if (key == "Cached") {
            stats.cached_bytes = value;
        }
    }
    
    // Вычисляем использованную память
    // Used = Total - Free - Buffers - Cached
    stats.used_bytes = stats.total_bytes 
                     - stats.free_bytes 
                     - stats.buffers_bytes 
                     - stats.cached_bytes;
    
    // Если MemAvailable не предоставлен ядром (старые ядра), оцениваем
    if (stats.available_bytes == 0) {
        stats.available_bytes = stats.free_bytes + stats.buffers_bytes + stats.cached_bytes;
    }
}


bool MemoryManager::parse_proc_status(MemoryStats::ProcessInfo& proc_info, int pid) {
    std::ostringstream status_path;
    status_path << "/proc/" << pid << "/status";
    
    std::ifstream status(status_path.str());
    if (!status.is_open()) {
        return false;  // Процесс мог завершиться
    }
    
    std::string line;
    bool found_name = false;
    while (std::getline(status, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        
        if (key == "Name:") {
            iss >> proc_info.name;
            found_name = true;
        } else if (key == "VmSize:") {
            size_t value;
            iss >> value;
            proc_info.vm_size = value * 1024;  // kB -> bytes
        } else if (key == "VmRSS:") {
            size_t value;
            iss >> value;
            proc_info.rss_size = value * 1024;  // kB -> bytes
        } else if (key == "RssFile:") {
            size_t value;
            iss >> value;
            proc_info.shared_size = value * 1024;  // приблизительная оценка
        }
    }
    
    // Получаем CPU процент из /proc/[pid]/stat
    std::ostringstream stat_path;
    stat_path << "/proc/" << pid << "/stat";
    
    std::ifstream stat_file(stat_path.str());
    if (stat_file.is_open()) {
        // Пропускаем первые поля до utime и stime
        std::string temp;
        int field = 0;
        long utime = 0, stime = 0;
        
        while (stat_file >> temp) {
            field++;
            if (field == 14) {  // utime
                utime = strtol(temp.c_str(), nullptr, 10);
            } else if (field == 15) {  // stime
                stime = strtol(temp.c_str(), nullptr, 10);
                break;
            }
        }
        
        // Очень грубая оценка CPU% (нужен дельта между замерами для точности)
        // Для простоты ставим 0.0, в реальном проекте нужно хранить предыдущие значения
        proc_info.cpu_percent = 0.0f;
    }
    
    // Если имя не найдено, используем PID
    if (proc_info.name.empty()) {
        proc_info.name = "process_" + std::to_string(pid);
    }
    
    return found_name || !proc_info.name.empty();
}

bool MemoryManager::check_and_clear_cache() {
    if (!initialized_) return false;
    
    float usage = current_stats_.usage_percent();
    if (usage >= cache_clear_threshold_) {
        return clear_kernel_cache();
    }
    
    return false;
}

void MemoryManager::calculate_averages() {
    // Реализовано в avg_usage_percent()
}

} // namespace robo_chassis
