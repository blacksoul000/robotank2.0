#include "system_monitor.hpp"
#include "../logger/logger.hpp"
#include "../sensors/sensor_manager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <vector>
#include <cstdio>
#include <sys/vfs.h>

namespace robo_chassis {

SystemMonitor::SystemMonitor() {
    LOG_INFO("SystemMonitor initialized");
    resetNetworkStats();
}

SystemMonitor::~SystemMonitor() {
    LOG_DEBUG("SystemMonitor destroyed");
}

bool SystemMonitor::update() {
    try {
        auto start = std::chrono::steady_clock::now();
        
        // Чтение всех метрик
        stats_.cpu_temperature_celsius = readCpuTemperature();
        stats_.cpu_usage_percent = readCpuUsage();
        stats_.cpu_frequency_mhz = readCpuFrequency();
        
        readMemoryStats();
        readDiskStats();
        readNetworkStats();
        readThrottlingStatus();
        
        // Обновление данных с датчиков
        auto& sensor_mgr = robo::SensorManager::getInstance();
        stats_.compass_ready = sensor_mgr.isCompassAvailable();
        stats_.ultrasonic_ready = sensor_mgr.isUltrasonicAvailable();
        
        if (stats_.ultrasonic_ready) {
            auto ultrasonic_data = sensor_mgr.getUltrasonicData();
            if (ultrasonic_data.valid) {
                stats_.distance_cm = ultrasonic_data.distance_cm;
            }
        }
        
        if (stats_.compass_ready) {
            auto compass_data = sensor_mgr.getCompassData();
            if (compass_data.valid) {
                stats_.compass_heading = compass_data.heading_deg;
            }
        }
        
        stats_.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        auto duration = std::chrono::steady_clock::now() - start;
        LOG_DEBUG_SRC("SystemMonitor update completed in " + 
                     std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) + " ms",
                     "system_monitor");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_SRC("SystemMonitor update failed: " + std::string(e.what()), "system_monitor");
        return false;
    }
}

bool SystemMonitor::needsThrottling() const {
    return stats_.is_throttled || 
           stats_.is_overheated ||
           stats_.cpu_temperature_celsius >= thresholds_.temperature_critical ||
           stats_.memory_usage_percent >= thresholds_.memory_critical;
}

std::vector<std::string> SystemMonitor::getRecommendations() const {
    std::vector<std::string> recommendations;
    
    if (stats_.cpu_temperature_celsius >= thresholds_.temperature_critical) {
        recommendations.push_back("CRITICAL: Температура CPU слишком высокая! Установите активное охлаждение.");
    } else if (stats_.cpu_temperature_celsius >= thresholds_.temperature_warning) {
        recommendations.push_back("WARNING: Температура CPU повышена. Проверьте охлаждение.");
    }
    
    if (stats_.is_under_voltage) {
        recommendations.push_back("CRITICAL: Пониженное напряжение! Используйте качественный блок питания (минимум 2.5A).");
    }
    
    if (stats_.is_freq_capped) {
        recommendations.push_back("WARNING: Частота CPU ограничена. Проверьте температуру и питание.");
    }
    
    if (stats_.memory_usage_percent >= thresholds_.memory_critical) {
        recommendations.push_back("CRITICAL: Критический уровень памяти! Закройте лишние процессы.");
    } else if (stats_.memory_usage_percent >= thresholds_.memory_warning) {
        recommendations.push_back("WARNING: Высокое использование памяти (>80%).");
    }
    
    if (stats_.cpu_usage_percent >= thresholds_.cpu_critical) {
        recommendations.push_back("CRITICAL: CPU загружен на 95%+. Уменьшите нагрузку.");
    } else if (stats_.cpu_usage_percent >= thresholds_.cpu_warning) {
        recommendations.push_back("WARNING: Высокая загрузка CPU (>80%).");
    }
    
    if (recommendations.empty()) {
        recommendations.push_back("Система работает в нормальном режиме.");
    }
    
    return recommendations;
}

void SystemMonitor::resetNetworkStats() {
    prev_rx_bytes_ = 0;
    prev_tx_bytes_ = 0;
    stats_.network_rx_bytes = 0;
    stats_.network_tx_bytes = 0;
    LOG_DEBUG("Network stats reset");
}

std::string SystemMonitor::toJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"cpu\":{\"usage_percent\":" << stats_.cpu_usage_percent 
         << ",\"temperature_c\":" << stats_.cpu_temperature_celsius
         << ",\"frequency_mhz\":" << stats_.cpu_frequency_mhz << "},";
    json << "\"memory\":{\"total_bytes\":" << stats_.memory_total_bytes
         << ",\"used_bytes\":" << stats_.memory_used_bytes
         << ",\"free_bytes\":" << stats_.memory_free_bytes
         << ",\"usage_percent\":" << stats_.memory_usage_percent << "},";
    json << "\"disk\":{\"total_bytes\":" << stats_.disk_total_bytes
         << ",\"used_bytes\":" << stats_.disk_used_bytes
         << ",\"free_bytes\":" << stats_.disk_free_bytes
         << ",\"usage_percent\":" << stats_.disk_usage_percent << "},";
    json << "\"network\":{\"rx_bytes\":" << stats_.network_rx_bytes
         << ",\"tx_bytes\":" << stats_.network_tx_bytes
         << ",\"wifi_quality\":" << stats_.wifi_link_quality << "},";
    json << "\"battery\":{\"voltage\":" << stats_.battery_voltage
         << ",\"low\":" << (stats_.battery_low ? "true" : "false") << "},";
    json << "\"sensors\":{\"arduino_connected\":" << (stats_.arduino_connected ? "true" : "false")
         << ",\"imu_ready\":" << (stats_.imu_ready ? "true" : "false")
         << ",\"compass_ready\":" << (stats_.compass_ready ? "true" : "false")
         << ",\"ultrasonic_ready\":" << (stats_.ultrasonic_ready ? "true" : "false")
         << ",\"distance_cm\":" << stats_.distance_cm
         << ",\"compass_heading\":" << stats_.compass_heading << "},";
    json << "\"throttling\":{\"is_throttled\":" << (stats_.is_throttled ? "true" : "false")
         << ",\"under_voltage\":" << (stats_.is_under_voltage ? "true" : "false")
         << ",\"freq_capped\":" << (stats_.is_freq_capped ? "true" : "false")
         << ",\"overheated\":" << (stats_.is_overheated ? "true" : "false") << "},";
    json << "\"timestamp_ms\":" << stats_.timestamp_ms;
    json << "}";
    return json.str();
}

float SystemMonitor::readCpuTemperature() const {
    // Чтение температуры с /sys/class/thermal/thermal_zone0/temp
    std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    if (!temp_file.is_open()) {
        LOG_WARNING_SRC("Cannot read CPU temperature", "system_monitor");
        return 0.0f;
    }
    
    int temp_raw;
    temp_file >> temp_raw;
    
    // Значение в миллиградусах, конвертируем в градусы
    float temp = temp_raw / 1000.0f;
    
    if (temp > 100.0f || temp < -10.0f) {
        LOG_WARNING_SRC("Invalid temperature reading: " + std::to_string(temp) + " C", "system_monitor");
        return 0.0f;
    }
    
    return temp;
}

float SystemMonitor::readCpuUsage() {
    // Чтение из /proc/stat для расчета загрузки CPU
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        LOG_WARNING("Cannot read /proc/stat");
        return 0.0f;
    }
    
    std::string line;
    if (!std::getline(stat_file, line)) {
        return 0.0f;
    }
    
    // Формат: cpu user nice system idle iowait irq softirq steal guest guest_nice
    std::istringstream iss(line);
    std::string cpu_label;
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    
    iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    uint64_t total_time = user + nice + system + idle + iowait + irq + softirq + steal;
    uint64_t idle_time = idle + iowait;
    
    float usage = 0.0f;
    if (prev_total_time_ > 0 && total_time > prev_total_time_) {
        uint64_t delta_total = total_time - prev_total_time_;
        uint64_t delta_idle = idle_time - prev_idle_time_;
        
        if (delta_total > 0) {
            usage = 100.0f * (1.0f - (static_cast<float>(delta_idle) / delta_total));
        }
    }
    
    prev_idle_time_ = idle_time;
    prev_total_time_ = total_time;
    
    // Ограничиваем диапазон 0-100
    return std::max(0.0f, std::min(100.0f, usage));
}

uint32_t SystemMonitor::readCpuFrequency() const {
    // Чтение текущей частоты CPU
    std::ifstream freq_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (!freq_file.is_open()) {
        // Пробуем альтернативный путь
        freq_file.open("/proc/cpuinfo");
        if (freq_file.is_open()) {
            std::string line;
            while (std::getline(freq_file, line)) {
                if (line.find("cpu MHz") != std::string::npos) {
                    size_t pos = line.find(':');
                    if (pos != std::string::npos) {
                        try {
                            return static_cast<uint32_t>(std::stof(line.substr(pos + 1)));
                        } catch (...) {
                            return 900; // Дефолт для RPi 2B
                        }
                    }
                }
            }
        }
        return 900; // Дефолт для RPi 2B (900 MHz)
    }
    
    uint32_t freq_khz;
    freq_file >> freq_khz;
    return freq_khz / 1000; // Конвертируем в MHz
}

void SystemMonitor::readMemoryStats() {
    auto total = parseMemInfo("MemTotal");
    auto free = parseMemInfo("MemFree");
    auto available = parseMemInfo("MemAvailable");
    auto buffers = parseMemInfo("Buffers");
    auto cached = parseMemInfo("Cached");
    
    if (!total.has_value()) {
        LOG_WARNING("Cannot read memory info");
        return;
    }
    
    stats_.memory_total_bytes = total.value() * 1024; // kB to bytes
    
    // MemAvailable есть не во всех ядрах, рассчитываем если нет
    uint64_t free_bytes = 0;
    if (available.has_value()) {
        free_bytes = available.value() * 1024;
    } else {
        free_bytes = (free.value_or(0) + buffers.value_or(0) + cached.value_or(0)) * 1024;
    }
    
    stats_.memory_free_bytes = free_bytes;
    stats_.memory_used_bytes = stats_.memory_total_bytes - stats_.memory_free_bytes;
    
    if (stats_.memory_total_bytes > 0) {
        stats_.memory_usage_percent = 100.0f * static_cast<float>(stats_.memory_used_bytes) / 
                                      static_cast<float>(stats_.memory_total_bytes);
    }
}

void SystemMonitor::readDiskStats() {
    // Используем statfs для получения информации о диске
    struct statfs buf;
    if (statfs("/", &buf) == 0) {
        stats_.disk_total_bytes = buf.f_blocks * buf.f_bsize;
        stats_.disk_free_bytes = buf.f_bavail * buf.f_bsize;
        stats_.disk_used_bytes = stats_.disk_total_bytes - stats_.disk_free_bytes;
        
        if (stats_.disk_total_bytes > 0) {
            stats_.disk_usage_percent = 100.0f * static_cast<float>(stats_.disk_used_bytes) / 
                                        static_cast<float>(stats_.disk_total_bytes);
        }
    } else {
        LOG_WARNING_SRC("Cannot read disk stats", "system_monitor");
    }
}

void SystemMonitor::readNetworkStats() {
    // Чтение статистики сети из /proc/net/dev
    std::ifstream net_file("/proc/net/dev");
    if (!net_file.is_open()) {
        return;
    }
    
    // Чтение WiFi качества (RSSI) для wlan0
    readWifiQuality();
    
    std::string line;
    uint64_t total_rx = 0, total_tx = 0;
    
    // Пропускаем заголовок (2 строки)
    std::getline(net_file, line);
    std::getline(net_file, line);
    
    while (std::getline(net_file, line)) {
        // Ищем интерфейсы eth0, wlan0 (игнорируем lo)
        if (line.find("lo:") != std::string::npos || 
            line.find("tun:") != std::string::npos ||
            line.find("docker") != std::string::npos) {
            continue;
        }
        
        // Формат: interface: rx_bytes rx_packets ... tx_bytes tx_packets ...
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string stats_part = line.substr(colon_pos + 1);
        std::istringstream iss(stats_part);
        
        uint64_t rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
        uint64_t tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
        
        if (iss >> rx_bytes >> rx_packets >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >> 
                rx_compressed >> rx_multicast >> tx_bytes >> tx_packets >> tx_errs >> tx_drop >> 
                tx_fifo >> tx_colls >> tx_carrier >> tx_compressed) {
            total_rx += rx_bytes;
            total_tx += tx_bytes;
        }
    }
    
    stats_.network_rx_bytes = total_rx;
    stats_.network_tx_bytes = total_tx;
}

void SystemMonitor::readWifiQuality() {
    // Чтение качества WiFi сигнала через /proc/net/wireless
    std::ifstream wireless_file("/proc/net/wireless");
    if (!wireless_file.is_open()) {
        stats_.wifi_rssi = 0;
        stats_.wifi_link_quality = 0;
        return;
    }
    
    std::string line;
    // Пропускаем заголовок (2 строки)
    std::getline(wireless_file, line);
    std::getline(wireless_file, line);
    
    while (std::getline(wireless_file, line)) {
        // Ищем интерфейс wlan0
        if (line.find("wlan0") == std::string::npos) {
            continue;
        }
        
        // Формат: wlan0: status link level noise nwid crypt frag retry misc beacon miss
        // Сдвиги:         0     1    2     3    4    5     6    7    8      9
        std::istringstream iss(line);
        std::string iface;
        int status, link, level, noise;
        
        iss >> iface >> status >> link >> level >> noise;
        
        if (iface.find("wlan0") != std::string::npos) {
            // level = RSSI в dBm (обычно -90..-30)
            // link = качество связи (0-100)
            stats_.wifi_rssi = level;
            stats_.wifi_link_quality = link;
            
            LOG_DEBUG("WiFi RSSI: " + std::to_string(level) + " dBm, Link: " + 
                     std::to_string(link) + "%");
            return;
        }
    }
    
    // Если wlan0 не найден, пробуем альтернативный метод через iwconfig
    FILE* pipe = popen("iwconfig 2>/dev/null | grep -A1 'wlan0' | grep 'Signal' | awk '{print $4}' | cut -d= -f2", "r");
    if (pipe) {
        char buffer[64];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            try {
                int rssi = std::stoi(std::string(buffer).substr(0, std::string(buffer).find_first_not_of("-0123456789\n\r") == std::string::npos ? std::string(buffer).length() : std::string(buffer).find_first_not_of("-0123456789\n\r")));
                stats_.wifi_rssi = rssi;
                // Конвертируем RSSI в процент (примерно)
                // -90 dBm = 0%, -50 dBm = 100%
                stats_.wifi_link_quality = std::max(0, std::min(100, (int)(2 * (rssi + 90))));
            } catch (...) {
                stats_.wifi_rssi = 0;
                stats_.wifi_link_quality = 0;
            }
        }
        pclose(pipe);
    }
}

void SystemMonitor::readThrottlingStatus() {
    // Чтение статуса троттлинга через vcgencmd get_throttled
    // Для RPi 2B это основной способ
    
    FILE* pipe = popen("/opt/vc/bin/vcgencmd get_throttled 2>/dev/null", "r");
    if (!pipe) {
        // Альтернатива: чтение из sysfs (новее ядра)
        std::ifstream throttle_file("/sys/devices/system/cpu/is_throttled");
        if (throttle_file.is_open()) {
            int throttled;
            throttle_file >> throttled;
            stats_.is_throttled = (throttled != 0);
        } else {
            LOG_DEBUG("Cannot read throttling status (vcgencmd not available)");
        }
        return;
    }
    
    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    // Формат: throttled=0xXXXXXXXX
    size_t eq_pos = result.find('=');
    if (eq_pos != std::string::npos) {
        try {
            uint32_t throttle_status = std::stoul(result.substr(eq_pos + 1), nullptr, 16);
            
            // Бит 0: Under-voltage detected
            stats_.is_under_voltage = (throttle_status & 0x1) != 0;
            // Бит 1: ARM frequency capped
            stats_.is_freq_capped = (throttle_status & 0x2) != 0;
            // Бит 2: Currently throttled
            stats_.is_throttled = (throttle_status & 0x4) != 0;
            // Бит 7: Temperature limit reached (overheated)
            stats_.is_overheated = (throttle_status & 0x80) != 0;
            
            if (stats_.is_throttled || stats_.is_under_voltage || stats_.is_overheated) {
                std::ostringstream hex_stream;
                hex_stream << std::hex << std::uppercase << throttle_status;
                std::string msg = "Throttling detected: status=0x" + hex_stream.str() + 
                                 ", under_voltage=" + (stats_.is_under_voltage ? "1" : "0") +
                                 ", freq_capped=" + (stats_.is_freq_capped ? "1" : "0") +
                                 ", throttled=" + (stats_.is_throttled ? "1" : "0") +
                                 ", overheated=" + (stats_.is_overheated ? "1" : "0");
                LOG_WARNING_SRC(msg, "system_monitor");
            }
        } catch (const std::exception& e) {
            LOG_WARNING_SRC("Failed to parse throttling status: " + std::string(e.what()), "system_monitor");
        }
    }
}

std::vector<std::string> SystemMonitor::readFileLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        return lines;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

std::optional<uint64_t> SystemMonitor::parseMemInfo(const std::string& label) {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        return std::nullopt;
    }
    
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find(label + ":") == 0) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                try {
                    return std::stoull(line.substr(colon_pos + 1));
                } catch (...) {
                    return std::nullopt;
                }
            }
        }
    }
    
    return std::nullopt;
}

} // namespace robo_chassis
