#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

class RobotLogic;

class TcpServer {
public:
    explicit TcpServer(int port, RobotLogic& robot);
    ~TcpServer();

    // Запуск сервера (блокирующий вызов)
    void run();
    
    // Остановка сервера
    void stop();
    
    // Получение количества подключенных клиентов
    size_t get_client_count() const;

private:
    int m_port;
    RobotLogic& m_robot;
    std::atomic<bool> m_running{false};
    int m_server_fd = -1;
    int m_epoll_fd = -1;
    
    // Максимальное количество одновременных подключений
    static constexpr int MAX_CLIENTS = 10;
    static constexpr int MAX_BUFFER_SIZE = 4096;
    
    // Rate limiting: макс. сообщений в секунду на клиента
    static constexpr int RATE_LIMIT_MAX_MSGS = 20;
    static constexpr int RATE_LIMIT_WINDOW_MS = 1000;
    
    struct ClientInfo {
        int fd;
        std::vector<char> buffer;
        std::chrono::steady_clock::time_point last_message_time;
        int message_count;
        
        ClientInfo(int client_fd) 
            : fd(client_fd), buffer(), last_message_time(std::chrono::steady_clock::now()), message_count(0) {}
    };
    
    std::unordered_map<int, std::unique_ptr<ClientInfo>> m_clients;
    mutable std::mutex m_clients_mutex;
    
    // Добавление клиента в epoll
    void add_client(int client_fd);
    
    // Удаление клиента
    void remove_client(int client_fd);
    
    // Обработка данных от клиента с rate limiting
    void handle_client_data(int client_fd, const char* data, size_t len);
    
    // Парсинг JSON команды (использует nlohmann/json)
    bool parse_command(const std::string& json, struct Command& cmd);
    
    // Проверка rate limit
    bool check_rate_limit(ClientInfo& client);
};
