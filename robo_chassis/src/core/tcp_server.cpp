#include "tcp_server.hpp"
#include "robot_logic.hpp"
#include "logger/logger.hpp"
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TcpServer::TcpServer(int port, RobotLogic& robot) 
    : m_port(port), m_robot(robot) {
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::run() {
    m_server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (m_server_fd == -1) {
        LOG_ERROR_SRC("Ошибка создания сокета", "tcp_server");
        return;
    }

    int opt = 1;
    setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);

    if (bind(m_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        LOG_ERROR_SRC("Ошибка привязки к порту " + std::to_string(m_port), "tcp_server");
        close(m_server_fd);
        return;
    }

    if (listen(m_server_fd, MAX_CLIENTS) < 0) {
        LOG_ERROR_SRC("Ошибка listen", "tcp_server");
        close(m_server_fd);
        return;
    }

    // Создаем epoll
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1) {
        LOG_ERROR_SRC("Ошибка создания epoll", "tcp_server");
        close(m_server_fd);
        return;
    }

    // Добавляем серверный сокет в epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = m_server_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_server_fd, &ev) < 0) {
        LOG_ERROR_SRC("Ошибка добавления серверного сокета в epoll", "tcp_server");
        close(m_epoll_fd);
        close(m_server_fd);
        return;
    }

    LOG_INFO_SRC("TCP сервер запущен на порту " + std::to_string(m_port) + " (epoll, макс. клиентов: " + std::to_string(MAX_CLIENTS) + ")", "tcp_server");
    m_running = true;

    std::vector<struct epoll_event> events(MAX_CLIENTS + 1);

    while (m_running) {
        int nfds = epoll_wait(m_epoll_fd, events.data(), static_cast<int>(events.size()), 1000);
        
        if (nfds == -1) {
            if (errno == EINTR) continue; // Прервано сигналом
            LOG_ERROR_SRC("Ошибка epoll_wait", "tcp_server");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == m_server_fd) {
                // Новое подключение
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int client_fd = accept(m_server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    if (errno == EINTR) continue;
                    LOG_WARNING_SRC("Ошибка accept", "tcp_server");
                    continue;
                }

                // Проверка лимита клиентов
                {
                    std::lock_guard<std::mutex> lock(m_clients_mutex);
                    if (m_clients.size() >= static_cast<size_t>(MAX_CLIENTS)) {
                        LOG_WARNING_SRC("Достигнут лимит клиентов (" + std::to_string(MAX_CLIENTS) + "), отклоняем подключение", "tcp_server");
                        close(client_fd);
                        continue;
                    }
                }

                // Устанавливаем non-blocking режим
                int flags = fcntl(client_fd, F_GETFL, 0);
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                LOG_INFO_SRC("Клиент подключен: " + std::string(inet_ntoa(client_addr.sin_addr)) + " (fd=" + std::to_string(client_fd) + ")", "tcp_server");
                add_client(client_fd);
            } else {
                // Данные от клиента или отключение
                if (events[i].events & (EPOLLIN | EPOLLRDHUP)) {
                    char buffer[MAX_BUFFER_SIZE];
                    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
                    
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0';
                        handle_client_data(fd, buffer, static_cast<size_t>(bytes_read));
                    } else if (bytes_read == 0 || (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                        // Клиент отключился
                        LOG_INFO_SRC("Клиент отключился (fd=" + std::to_string(fd) + ")", "tcp_server");
                        remove_client(fd);
                    }
                }
                
                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    LOG_INFO_SRC("Ошибка подключения клиента (fd=" + std::to_string(fd) + ")", "tcp_server");
                    remove_client(fd);
                }
            }
        }
    }

    // Очистка всех клиентов
    {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        for (auto& [fd, client] : m_clients) {
            close(fd);
        }
        m_clients.clear();
    }

    if (m_epoll_fd != -1) {
        close(m_epoll_fd);
        m_epoll_fd = -1;
    }
    
    if (m_server_fd != -1) {
        close(m_server_fd);
        m_server_fd = -1;
    }
}

void TcpServer::stop() {
    m_running = false;
}

size_t TcpServer::get_client_count() const {
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    return m_clients.size();
}

void TcpServer::add_client(int client_fd) {
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    
    auto client_info = std::make_unique<ClientInfo>(client_fd);
    client_info->buffer.reserve(MAX_BUFFER_SIZE);
    
    m_clients[client_fd] = std::move(client_info);
    
    // Добавляем в epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
}

void TcpServer::remove_client(int client_fd) {
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    
    auto it = m_clients.find(client_fd);
    if (it != m_clients.end()) {
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        close(client_fd);
        m_clients.erase(it);
        LOG_INFO_SRC("Клиент удален (fd=" + std::to_string(client_fd) + "), осталось клиентов: " + std::to_string(m_clients.size()), "tcp_server");
    }
}

bool TcpServer::check_rate_limit(ClientInfo& client) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - client.last_message_time).count();
    
    if (elapsed >= RATE_LIMIT_WINDOW_MS) {
        // Сброс счетчика после истечения окна
        client.message_count = 0;
        client.last_message_time = now;
    }
    
    if (client.message_count >= RATE_LIMIT_MAX_MSGS) {
        return false; // Превышен лимит
    }
    
    client.message_count++;
    return true;
}

void TcpServer::handle_client_data(int client_fd, const char* data, size_t len) {
    ClientInfo* client = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        auto it = m_clients.find(client_fd);
        if (it == m_clients.end()) return;
        client = it->second.get();
    }
    
    // Проверка rate limit
    if (!check_rate_limit(*client)) {
        LOG_WARNING_SRC("Rate limit превышен для клиента fd=" + std::to_string(client_fd), "tcp_server");
        return;
    }
    
    // Добавляем данные в буфер
    client->buffer.insert(client->buffer.end(), data, data + len);
    
    // Обрабатываем полные строки (команды разделены \n)
    while (true) {
        auto pos = std::find(client->buffer.begin(), client->buffer.end(), '\n');
        if (pos == client->buffer.end()) break;
        
        std::string line(client->buffer.begin(), pos);
        
        // Удаляем \r если есть
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Удаляем обработанную строку из буфера
        client->buffer.erase(client->buffer.begin(), pos + 1);
        
        if (line.empty()) continue;
        
        Command cmd;
        if (parse_command(line, cmd)) {
            m_robot.process_command(cmd);
        } else {
            LOG_WARNING_SRC("Неверный формат команды: " + line, "tcp_server");
        }
    }
}

// Парсер JSON с использованием nlohmann/json для безопасности и надёжности
bool TcpServer::parse_command(const std::string& json_str, Command& cmd) {
    try {
        // Используем безопасный парсер nlohmann/json
        json j = json::parse(json_str);
        
        // Извлекаем значения с проверкой типа
        if (j.contains("left_x") && j["left_x"].is_number()) {
            cmd.left_x = j["left_x"].get<float>();
        }
        
        if (j.contains("left_y") && j["left_y"].is_number()) {
            cmd.left_y = j["left_y"].get<float>();
        }
        
        if (j.contains("right_x") && j["right_x"].is_number()) {
            cmd.right_x = j["right_x"].get<float>();
        }
        
        if (j.contains("right_y") && j["right_y"].is_number()) {
            cmd.right_y = j["right_y"].get<float>();
        }
        
        if (j.contains("fire") && j["fire"].is_boolean()) {
            cmd.fire = j["fire"].get<bool>();
        }
        
        if (j.contains("lights") && j["lights"].is_boolean()) {
            cmd.lights = j["lights"].get<bool>();
        }
        
        return true;
    } catch (const json::parse_error& e) {
        LOG_WARNING_SRC("Ошибка парсинга JSON: " + std::string(e.what()), "tcp_server");
        return false;
    } catch (const json::type_error& e) {
        LOG_WARNING_SRC("Ошибка типа JSON: " + std::string(e.what()), "tcp_server");
        return false;
    } catch (const std::exception& e) {
        LOG_WARNING_SRC("Неизвестная ошибка при парсинге: " + std::string(e.what()), "tcp_server");
        return false;
    }
}
