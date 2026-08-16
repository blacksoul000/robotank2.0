#include "safety_manager.hpp"
#include "../logger/logger.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <cstdlib>

namespace robo_chassis {

SafetyManager* SafetyManager::s_instance = nullptr;

SafetyManager::SafetyManager(int watchdog_timeout_sec, int arduino_timeout_sec)
    : m_watchdog_timeout_sec(watchdog_timeout_sec)
    , m_arduino_timeout_sec(arduino_timeout_sec)
    , m_last_pet_time(std::chrono::steady_clock::now())
    , m_last_arduino_contact(std::chrono::steady_clock::now())
{
    s_instance = this;
}

SafetyManager::~SafetyManager() {
    stop();
    s_instance = nullptr;
}

bool SafetyManager::init() {
    LOG_INFO("SafetyManager: Инициализация...");
    
    // Попытка открыть аппаратный watchdog
    if (openHardwareWatchdog()) {
        LOG_INFO("SafetyManager: Аппаратный watchdog успешно инициализирован (/dev/watchdog)");
    } else {
        LOG_WARNING("SafetyManager: Не удалось открыть /dev/watchdog, используется только программный watchdog");
    }
    
    // Установка обработчиков сигналов
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        LOG_ERROR("SafetyManager: Не удалось установить обработчик SIGINT");
        return false;
    }
    
    if (sigaction(SIGTERM, &sa, nullptr) < 0) {
        LOG_ERROR("SafetyManager: Не удалось установить обработчик SIGTERM");
        return false;
    }
    
    LOG_INFO("SafetyManager: Обработчики сигналов установлены");
    return true;
}

void SafetyManager::start() {
    if (m_running.load()) {
        LOG_WARNING("SafetyManager: Уже запущен");
        return;
    }
    
    m_running = true;
    m_watchdog_thread = std::thread(&SafetyManager::watchdogThread, this);
    LOG_INFO("SafetyManager: Watchdog поток запущен");
}

void SafetyManager::stop() {
    if (!m_running.load()) {
        return;
    }
    
    LOG_INFO("SafetyManager: Остановка...");
    m_running = false;
    
    if (m_watchdog_thread.joinable()) {
        m_watchdog_thread.join();
    }
    
    closeHardwareWatchdog();
    LOG_INFO("SafetyManager: Остановлен");
}

void SafetyManager::petWatchdog() {
    m_last_pet_time = std::chrono::steady_clock::now();
    m_watchdog_counter = 0;
    
    // Сброс аппаратного watchdog
    if (m_watchdog_fd >= 0) {
        char dummy = 'X';
        if (write(m_watchdog_fd, &dummy, 1) < 0) {
            LOG_WARNING("SafetyManager: Ошибка записи в /dev/watchdog");
        }
    }
}

void SafetyManager::updateArduinoStatus(bool online) {
    if (online) {
        m_last_arduino_contact = std::chrono::steady_clock::now();
        m_arduino_loss_counter = 0;
        
        // Автоматический выход из безопасного режима при восстановлении связи
        if (m_safe_mode_active.load() && m_emergency_stop_callback) {
            LOG_INFO("SafetyManager: Связь с Arduino восстановлена. Выход из безопасного режима.");
            deactivateSafeMode();
        }
    } else {
        m_arduino_loss_counter++;
        
        // Активация безопасного режима при потере связи
        if (m_arduino_loss_counter >= m_arduino_timeout_sec && !m_safe_mode_active.load()) {
            LOG_ERROR("SafetyManager: Потеря связи с Arduino более " + 
                     std::to_string(m_arduino_timeout_sec) + " сек. Активация безопасного режима!");
            activateSafeMode();
        }
    }
}

void SafetyManager::setEmergencyStopCallback(std::function<void()> callback) {
    m_emergency_stop_callback = std::move(callback);
}

void SafetyManager::activateSafeMode() {
    m_safe_mode_active = true;
    
    if (m_emergency_stop_callback) {
        m_emergency_stop_callback();
        LOG_INFO("SafetyManager: Двигатели остановлены (безопасный режим)");
    }
}

void SafetyManager::deactivateSafeMode() {
    m_safe_mode_active = false;
    LOG_INFO("SafetyManager: Безопасный режим деактивирован");
}

void SafetyManager::watchdogThread() {
    LOG_INFO("SafetyManager: Watchdog поток начал работу");
    
    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        auto now = std::chrono::steady_clock::now();
        
        // Проверка программного watchdog (петинг)
        auto time_since_pet = std::chrono::duration_cast<std::chrono::seconds>(
            now - m_last_pet_time).count();
        
        if (time_since_pet > m_watchdog_timeout_sec) {
            m_watchdog_counter++;
            
            if (m_watchdog_counter >= m_watchdog_timeout_sec) {
                LOG_CRITICAL("SafetyManager: Критическое зависание системы! Требуется перезагрузка.");
                m_needs_reboot = true;
                
                // Попытка программной перезагрузки
                LOG_INFO("SafetyManager: Выполнение перезагрузки системы...");
                sync();  // Сброс буферов диска
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // Попытка через syscall
                reboot(RB_AUTOBOOT);
                
                // Если reboot не сработал, пробуем альтернативный метод
                LOG_ERROR("SafetyManager: reboot() не сработал, пробуем system()");
                std::system("sudo reboot");
                
                break;
            }
        } else {
            m_watchdog_counter = 0;
        }
        
        // Дополнительная проверка на полное зависание
        auto time_since_arduino = std::chrono::duration_cast<std::chrono::seconds>(
            now - m_last_arduino_contact).count();
        
        if (time_since_arduino > m_watchdog_timeout_sec * 2 && !m_safe_mode_active.load()) {
            LOG_ERROR("SafetyManager: Критическое время без связи с Arduino! Активация safe mode.");
            activateSafeMode();
        }
    }
    
    LOG_INFO("SafetyManager: Watchdog поток завершён");
}

void SafetyManager::signalHandler(int signum) {
    LOG_INFO("SafetyManager: Получен сигнал " + std::to_string(signum) + ". Запуск graceful shutdown...");
    
    if (s_instance) {
        s_instance->gracefulShutdown();
    }
}

void SafetyManager::gracefulShutdown() {
    LOG_INFO("SafetyManager: Выполнение graceful shutdown...");
    
    // Остановка двигателей
    activateSafeMode();
    
    // Закрытие watchdog перед выходом (чтобы не перезагрузиться)
    closeHardwareWatchdog();
    
    LOG_INFO("SafetyManager: Graceful shutdown завершён");
}

bool SafetyManager::openHardwareWatchdog() {
    m_watchdog_fd = open("/dev/watchdog", O_WRONLY);
    
    if (m_watchdog_fd < 0) {
        return false;
    }
    
    // Магическое отключение при закрытии (Linux watchdog)
    // Чтобы система не перезагрузилась при нормальном закрытии
    LOG_INFO("SafetyManager: /dev/watchdog открыт (fd=" + std::to_string(m_watchdog_fd) + ")");
    return true;
}

void SafetyManager::closeHardwareWatchdog() {
    if (m_watchdog_fd >= 0) {
        // Отправляем 'V' (magic character) чтобы предотвратить перезагрузку
        // Это стандартный механизм Linux watchdog
        char magic = 'V';
        write(m_watchdog_fd, &magic, 1);
        close(m_watchdog_fd);
        m_watchdog_fd = -1;
        LOG_INFO("SafetyManager: /dev/watchdog закрыт");
    }
}

} // namespace robo_chassis
