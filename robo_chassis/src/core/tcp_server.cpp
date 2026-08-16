#include "tcp_server.hpp"
#include "robot_logic.hpp"
#include "logger/logger.hpp"
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TcpServer::TcpServer(int port, RobotLogic& robot) 
    : m_port(port), m_robot(robot) {
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::run() {
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(m_server_fd, 3) < 0) {
        LOG_ERROR_SRC("Ошибка listen", "tcp_server");
        close(m_server_fd);
        return;
    }

    LOG_INFO_SRC("TCP сервер запущен на порту " + std::to_string(m_port), "tcp_server");
    m_running = true;

    while (m_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(m_server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue; // Прервано сигналом
            break;
        }

        LOG_INFO_SRC("Клиент подключен: " + std::string(inet_ntoa(client_addr.sin_addr)), "tcp_server");
        handle_client(client_fd);
        close(client_fd);
    }

    close(m_server_fd);
    m_server_fd = -1;
}

void TcpServer::stop() {
    m_running = false;
    if (m_server_fd != -1) {
        shutdown(m_server_fd, SHUT_RDWR);
    }
}

void TcpServer::handle_client(int client_fd) {
    char buffer[1024] = {0};
    
    while (m_running) {
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            if (bytes_read < 0 && errno == EAGAIN) {
                usleep(1000);
                continue;
            }
            break; // Клиент отключился
        }

        buffer[bytes_read] = '\0';
        std::string line(buffer);
        
        // Удаляем завершающие \r\n
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }

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
