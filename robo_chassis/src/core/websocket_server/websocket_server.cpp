#include "websocket_server.hpp"
#include "../robot_logic.hpp"
#include "../logger/logger.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace robo_chassis {

// Base64 encoding для WebSocket handshake
static std::string base64_encode(const unsigned char* data, size_t len) {
    static const char* b64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    while (len--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + 
                             ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + 
                             ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++)
                result += b64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + 
                         ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + 
                         ((char_array_3[2] & 0xc0) >> 6);
        
        for (int j = 0; j < i + 1; j++)
            result += b64_chars[char_array_4[j]];
        
        while (i++ < 3)
            result += '=';
    }
    
    return result;
}

WebSocketServer::WebSocketServer(int port) : m_port(port) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    if (m_running.load()) {
        LOG_WARNING("WebSocket сервер уже запущен");
        return;
    }
    
    m_running = true;
    m_server_thread = std::thread(&WebSocketServer::serverLoop, this);
    LOG_INFO("WebSocket сервер запущен на порту " + std::to_string(m_port));
}

void WebSocketServer::stop() {
    if (!m_running.load()) return;
    
    m_running = false;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (int client_fd : m_clients) {
            if (client_fd >= 0) {
                shutdown(client_fd, SHUT_RDWR);
                close(client_fd);
            }
        }
        m_clients.clear();
    }
    
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }
    
    LOG_INFO("WebSocket сервер остановлен");
}

int WebSocketServer::getClientCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_clients.size());
}

void WebSocketServer::setCommandCallback(std::function<void(const Command&)> callback) {
    m_command_callback = std::move(callback);
}

void WebSocketServer::setAutonomyCallback(std::function<void(const std::string&)> callback) {
    m_autonomy_callback = std::move(callback);
}

void WebSocketServer::serverLoop() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        LOG_CRITICAL("Ошибка создания сокета WebSocket сервера");
        return;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        LOG_CRITICAL("Ошибка привязки WebSocket сервера к порту " + std::to_string(m_port));
        close(server_fd);
        return;
    }
    
    if (listen(server_fd, 5) < 0) {
        LOG_CRITICAL("Ошибка listen WebSocket сервера");
        close(server_fd);
        return;
    }
    
    LOG_INFO("Ожидание WebSocket подключений...");
    
    while (m_running.load()) {
        struct sockaddr_in client_addr {};
        socklen_t client_len = sizeof(client_addr);
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        struct timeval timeout {1, 0};
        
        int activity = select(server_fd + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                LOG_INFO("Новое WebSocket подключение: " + 
                        std::string(inet_ntoa(client_addr.sin_addr)));
                
                if (performHandshake(client_fd)) {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_clients.push_back(client_fd);
                    std::thread(&WebSocketServer::handleClient, this, client_fd).detach();
                } else {
                    close(client_fd);
                }
            }
        }
    }
    
    close(server_fd);
}

void WebSocketServer::handleClient(int client_fd) {
    unsigned char buffer[4096];
    
    // Инициализация rate limit для нового клиента
    {
        std::lock_guard<std::mutex> lock(m_rate_limit_mutex);
        m_client_rate_limits[client_fd] = ClientRateLimit{};
    }
    
    while (m_running.load()) {
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
        
        if (bytes_read <= 0) break;
        
        // Проверка rate limit перед обработкой сообщения
        if (!checkRateLimit(client_fd)) {
            LOG_WARNING("Rate limit превышен для клиента " + std::to_string(client_fd));
            continue;  // Пропускаем сообщение, но не закрываем соединение
        }
        
        std::string payload;
        bool is_text = false;
        
        if (parseWebSocketFrame(buffer, bytes_read, payload, is_text)) {
            if (is_text && !payload.empty()) {
                LOG_DEBUG("Получена команда: " + payload);
                
                Command cmd;
                auto findValue = [&](const std::string& key) -> std::string {
                    size_t pos = payload.find("\"" + key + "\"");
                    if (pos == std::string::npos) return "";
                    pos = payload.find(':', pos);
                    if (pos == std::string::npos) return "";
                    pos++;
                    while (pos < payload.size() && 
                           (payload[pos] == ' ' || payload[pos] == '\t')) pos++;
                    if (pos >= payload.size()) return "";
                    
                    size_t end = pos;
                    if (payload[pos] == '"') {
                        end = payload.find('"', pos + 1);
                        if (end == std::string::npos) return "";
                        return payload.substr(pos + 1, end - pos - 1);
                    } else {
                        while (end < payload.size() && 
                               payload[end] != ',' && payload[end] != '}') {
                            end++;
                        }
                        return payload.substr(pos, end - pos);
                    }
                };
                
                try {
                    std::string val;
                    val = findValue("left_x");
                    if (!val.empty()) cmd.left_x = std::stof(val);
                    
                    val = findValue("left_y");
                    if (!val.empty()) cmd.left_y = std::stof(val);
                    
                    val = findValue("right_x");
                    if (!val.empty()) cmd.right_x = std::stof(val);
                    
                    val = findValue("right_y");
                    if (!val.empty()) cmd.right_y = std::stof(val);
                    
                    val = findValue("fire");
                    cmd.fire = (val == "true");
                    
                    val = findValue("lights");
                    cmd.lights = (val == "true");
                    
                    if (m_command_callback) {
                        m_command_callback(cmd);
                    }
                } catch (...) {
                    LOG_WARNING("Ошибка парсинга команды");
                }
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find(m_clients.begin(), m_clients.end(), client_fd);
        if (it != m_clients.end()) m_clients.erase(it);
    }
    
    // Очистка rate limit для отключившегося клиента
    cleanupClientRateLimit(client_fd);
    
    close(client_fd);
    LOG_INFO("Клиент WebSocket отключился");
}

bool WebSocketServer::parseWebSocketFrame(const unsigned char* buffer, size_t len,
                                         std::string& payload, bool& is_text) {
    if (len < 2) return false;
    
    uint8_t opcode = buffer[0] & 0x0F;
    bool masked = (buffer[1] & 0x80) != 0;
    uint64_t payload_len = buffer[1] & 0x7F;
    
    is_text = (opcode == 0x01);
    
    size_t header_size = 2;
    if (payload_len == 126) {
        if (len < 4) return false;
        payload_len = (buffer[2] << 8) | buffer[3];
        header_size = 4;
    } else if (payload_len == 127) {
        if (len < 10) return false;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | buffer[2 + i];
        }
        header_size = 10;
    }
    
    unsigned char mask[4] = {0};
    if (masked) {
        if (len < header_size + 4) return false;
        memcpy(mask, buffer + header_size, 4);
        header_size += 4;
    }
    
    if (len < header_size + payload_len) return false;
    
    payload.resize(payload_len);
    for (size_t i = 0; i < payload_len; i++) {
        payload[i] = masked ? (buffer[header_size + i] ^ mask[i % 4]) 
                           : buffer[header_size + i];
    }
    
    return true;
}

std::vector<unsigned char> WebSocketServer::createWebSocketFrame(const std::string& payload, 
                                                                bool is_text) {
    std::vector<unsigned char> frame;
    
    uint8_t opcode = is_text ? 0x01 : 0x02;
    size_t len = payload.size();
    
    frame.push_back(0x80 | opcode);
    
    if (len <= 125) {
        frame.push_back(len);
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    
    for (char c : payload) {
        frame.push_back(static_cast<unsigned char>(c));
    }
    
    return frame;
}

bool WebSocketServer::performHandshake(int client_fd) {
    char buffer[4096] = {0};
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        LOG_WARNING("Не удалось прочитать данные для handshake");
        return false;
    }
    
    std::string request(buffer);
    
    size_t key_pos = request.find("Sec-WebSocket-Key: ");
    if (key_pos == std::string::npos) {
        LOG_WARNING("Sec-WebSocket-Key не найден");
        return false;
    }
    
    size_t key_end = request.find("\r\n", key_pos);
    std::string key = request.substr(key_pos + 19, key_end - key_pos - 21);
    
    const std::string ws_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string accept_key = base64_encode(
        reinterpret_cast<const unsigned char*>((key + ws_guid).c_str()),
        (key + ws_guid).size());
    
    std::string response = 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept_key + "\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n";
    
    if (write(client_fd, response.c_str(), response.size()) < 0) {
        LOG_WARNING("Не удалось отправить ответ handshake");
        return false;
    }
    
    LOG_INFO("WebSocket handshake успешен");
    return true;
}

void WebSocketServer::broadcastTelemetry(const Telemetry& telemetry, 
                                        float cpu_temp, 
                                        float memory_percent,
                                        float heading,
                                        float mag_x,
                                        float mag_y,
                                        float mag_z,
                                        float ultrasonic_dist,
                                        int wifi_quality) {
    std::ostringstream json;
    json << "{\"type\":\"TELEMETRY\","
         << "\"battery\":" << telemetry.battery_voltage << ","
         << "\"roll\":" << telemetry.roll << ","
         << "\"pitch\":" << telemetry.pitch << ","
         << "\"yaw\":" << telemetry.yaw << ","
         << "\"turret_angle\":" << telemetry.turret_angle << ","
         << "\"current_left\":" << telemetry.current_left << ","
         << "\"current_right\":" << telemetry.current_right << ","
         << "\"current_tower\":" << telemetry.current_tower << ","
         << "\"signal_quality\":" << telemetry.signal_quality << ","
         << "\"arduino_online\":" << (telemetry.arduino_online ? "true" : "false") << ","
         << "\"gyro_ready\":" << (telemetry.gyro_ready ? "true" : "false") << ","
         << "\"cpu_temp\":" << cpu_temp << ","
         << "\"memory_percent\":" << memory_percent << ","
         << "\"heading\":" << heading << ","
         << "\"mag_x\":" << mag_x << ","
         << "\"mag_y\":" << mag_y << ","
         << "\"mag_z\":" << mag_z << ","
         << "\"ultrasonic_cm\":" << ultrasonic_dist << ","
         << "\"wifi_quality\":" << wifi_quality << "}";
    
    std::string message = json.str();
    auto frame = createWebSocketFrame(message, true);
    
    // Копируем список клиентов под блокировкой для минимизации времени удержания мьютекса
    std::vector<int> clients_copy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        clients_copy = m_clients;
    }
    
    // Отправка данных без удержания мьютекса (чтобы не блокировать новые подключения)
    std::vector<int> disconnected_clients;
    for (int client_fd : clients_copy) {
        if (client_fd >= 0) {
            ssize_t bytes_sent = write(client_fd, frame.data(), frame.size());
            if (bytes_sent < 0) {
                // Ошибка записи - клиент отключился
                LOG_DEBUG("Ошибка записи клиенту " + std::to_string(client_fd) + ", будет удалён");
                disconnected_clients.push_back(client_fd);
            }
        }
    }
    
    // Удаление отключившихся клиентов
    if (!disconnected_clients.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (int client_fd : disconnected_clients) {
            auto it = std::find(m_clients.begin(), m_clients.end(), client_fd);
            if (it != m_clients.end()) {
                m_clients.erase(it);
                cleanupClientRateLimit(client_fd);
                close(client_fd);
                LOG_INFO("Клиент удалён из-за ошибки записи: " + std::to_string(client_fd));
            }
        }
    }
}

// Реализация проверки rate limit
bool WebSocketServer::checkRateLimit(int client_fd) {
    std::lock_guard<std::mutex> lock(m_rate_limit_mutex);
    
    auto it = m_client_rate_limits.find(client_fd);
    if (it == m_client_rate_limits.end()) {
        return true;  // Клиент не найден, разрешаем
    }
    
    auto now = std::chrono::steady_clock::now();
    auto& limit = it->second;
    
    // Сброс счётчика если прошла секунда
    if (now - limit.last_message_time >= ClientRateLimit::WINDOW_SEC) {
        limit.message_count = 0;
        limit.last_message_time = now;
    }
    
    // Проверка лимита
    if (limit.message_count >= ClientRateLimit::MAX_MESSAGES_PER_SECOND) {
        return false;  // Превышен лимит
    }
    
    limit.message_count++;
    return true;
}

// Очистка rate limits при отключении клиента
void WebSocketServer::cleanupClientRateLimit(int client_fd) {
    std::lock_guard<std::mutex> lock(m_rate_limit_mutex);
    m_client_rate_limits.erase(client_fd);
}

} // namespace robo_chassis
