#include <iostream>
#include <thread>
#include <vector>
#include <csignal>
#include <atomic>

#include "tcp_server.hpp"
#include "serial_port.hpp"
#include "robot_logic.hpp"

std::atomic<bool> g_running{true};

void signal_handler(int signum) {
    std::cout << "\nПолучен сигнал остановки (" << signum << "). Завершение работы...\n";
    g_running = false;
}

int main() {
    std::cout << "=== RoboChassis Core (Pure C++20) ===\n";
    
    // Установка обработчика сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Инициализация компонентов
        SerialPort serial("/dev/ttyUSB0", 115200); // Путь к Arduino может отличаться
        RobotLogic robot(serial);
        TcpServer server(5555, robot);

        std::cout << "Запуск основного цикла...\n";
        std::cout << "Ожидание команд от Python Bridge на порту 5555...\n";

        // Запуск TCP сервера в отдельном потоке
        std::thread server_thread([&server]() {
            server.run();
        });

        // Основной цикл обработки телеметрии и логики
        while (g_running) {
            robot.update_telemetry();
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 50 Hz
        }

        std::cout << "Остановка сервера...\n";
        server.stop();
        
        if (server_thread.joinable()) {
            server_thread.join();
        }

        std::cout << "Работа завершена.\n";

    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
