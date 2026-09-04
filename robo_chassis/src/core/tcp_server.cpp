#include "tcp_server.hpp"
#include "robot_logic.hpp"
#include "logger.hpp"
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

// Очень простой парсер JSON без внешних библиотек
// Ожидает формат: {"left_x":0.5,"left_y":-0.3,"right_x":0,"right_y":0.8,"fire":false}
bool TcpServer::parse_command(const std::string& json, Command& cmd) {
    auto find_value = [&](const std::string& key) -> std::string {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        
        pos++; // пропуск ':'
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        if (pos >= json.size()) return "";
        
        size_t end = pos;
        if (json[pos] == '"') {
            end = json.find('"', pos + 1);
            if (end == std::string::npos) return "";
            return json.substr(pos + 1, end - pos - 1);
        } else {
            while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != '\n') {
                end++;
            }
            return json.substr(pos, end - pos);
        }
    };

    try {
        std::string val;
        
        val = find_value("left_x");
        if (!val.empty()) cmd.left_x = std::stof(val);
        
        val = find_value("left_y");
        if (!val.empty()) cmd.left_y = std::stof(val);
        
        val = find_value("right_x");
        if (!val.empty()) cmd.right_x = std::stof(val);
        
        val = find_value("right_y");
        if (!val.empty()) cmd.right_y = std::stof(val);
        
        val = find_value("fire");
        cmd.fire = (val == "true");
        
        val = find_value("lights");
        cmd.lights = (val == "true");

        return true;
    } catch (...) {
        return false;
    }
}
