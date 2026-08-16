#pragma once

#include <string>
#include <atomic>
#include <memory>

class RobotLogic;

class TcpServer {
public:
    explicit TcpServer(int port, RobotLogic& robot);
    ~TcpServer();

    // Запуск сервера (блокирующий вызов)
    void run();
    
    // Остановка сервера
    void stop();

private:
    int m_port;
    RobotLogic& m_robot;
    std::atomic<bool> m_running{false};
    int m_server_fd = -1;
    
    // Обработка одного клиента
    void handle_client(int client_fd);
    
    // Парсинг JSON команды (упрощенный)
    bool parse_command(const std::string& json, struct Command& cmd);
};
