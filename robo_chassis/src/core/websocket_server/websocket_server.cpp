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
#include <nlohmann/json.hpp>
#include <openssl/sha.h>

using json = nlohmann::json;

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

// SHA-1 hash для WebSocket handshake
static std::string sha1_hash(const std::string& input) {
    const char* guide = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = input + guide;
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.size(), hash);
    
    return base64_encode(hash, SHA_DIGEST_LENGTH);
}

WebSocketServer::WebSocketServer(int port) : m_port(port), m_server_fd(-1) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    if (m_running.load()) {
        LOG_WARNING("WebSocket сервер уже запущен");
        return;
    }
    
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_fd < 0) {
        LOG_ERROR("Не удалось создать сокет WebSocket сервера");
        return;
    }
    
    int opt = 1;
    setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);
    
    if (bind(m_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Не удалось выполнить bind на порту {}", m_port);
        close(m_server_fd);
        m_server_fd = -1;
        return;
    }
    
    if (listen(m_server_fd, 10) < 0) {
        LOG_ERROR("Не удалось выполнить listen");
        close(m_server_fd);
        m_server_fd = -1;
        return;
    }
    
    m_running = true;
    m_server_thread = std::thread(&WebSocketServer::serverLoop, this);
    
    LOG_INFO("WebSocket сервер запущен на порту {}", m_port);
    LOG_INFO("Ожидание WebSocket подключений...");
}

void WebSocketServer::stop() {
    if (!m_running.exchange(false)) {
        // Сервер уже остановлен
        return;
    }

    // Закрываем серверный сокет для прерывания accept() в serverLoop
    if (m_server_fd != -1) {
        shutdown(m_server_fd, SHUT_RDWR);
        close(m_server_fd);
        m_server_fd = -1;
    }

    // Graceful shutdown всех клиентских соединений
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& client : m_clients) {
            if (client && client->fd != -1) {
                shutdown(client->fd, SHUT_RDWR);
                close(client->fd);
                client->fd = -1;
            }
        }
        m_clients.clear();
    }

    // Присоединяем все потоки клиентов для предотвращения утечек
    // Это критически важно для proper cleanup
    {
        std::lock_guard<std::mutex> thread_lock(m_client_threads_mutex);
        for (auto& thread : m_client_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        m_client_threads.clear();
    }

    // Присоединяем поток сервера
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }

    // Очистка rate limit данных
    {
        std::lock_guard<std::mutex> lock(m_rate_limit_mutex);
        m_client_rate_limits.clear();
    }

    LOG_INFO("WebSocket сервер остановлен");
}

int WebSocketServer::getClientCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_clients.size());
}

bool WebSocketServer::checkRateLimit(int client_fd) {
    std::lock_guard<std::mutex> lock(m_rate_limit_mutex);
    
    auto now = std::chrono::steady_clock::now();
    auto& limit = m_client_rate_limits[client_fd];
    
    if (now - limit.last_message_time > ClientRateLimit::WINDOW_SEC) {
        limit.last_message_time = now;
        limit.message_count = 1;
        return true;
    }
    
    limit.message_count++;
    if (limit.message_count > ClientRateLimit::MAX_MESSAGES_PER_SECOND) {
        LOG_WARNING("Rate limit превышен для клиента {}", client_fd);
        return false;
    }
    
    return true;
}

void WebSocketServer::cleanupClientRateLimit(int client_fd) {
    std::lock_guard<std::mutex> lock(m_rate_limit_mutex);
    m_client_rate_limits.erase(client_fd);
}

void WebSocketServer::serverLoop() {
    int server_fd = m_server_fd;
    
    while (m_running.load()) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        
        int activity = select(server_fd + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                LOG_INFO("Новое WebSocket подключение: " + 
                        std::string(inet_ntoa(client_addr.sin_addr)));
                
                if (performHandshake(client_fd)) {
                    auto client = std::make_unique<WslayClient>();
                    client->fd = client_fd;
                    client->server = this;  // Устанавливаем указатель на сервер
                    
                    struct wslay_event_callbacks callbacks {};
                    callbacks.recv_callback = recv_callback;
                    callbacks.send_callback = send_callback;
                    callbacks.genmask_callback = nullptr;
                    callbacks.on_frame_recv_start_callback = nullptr;
                    callbacks.on_frame_recv_chunk_callback = nullptr;
                    callbacks.on_frame_recv_end_callback = nullptr;
                    callbacks.on_msg_recv_callback = on_msg_recv_callback;
                    
                    wslay_event_context* ctx;
                    if (wslay_event_context_server_init(&ctx, &callbacks, client.get()) == 0) {
                        client->ctx = ctx;
                        
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_clients.push_back(std::move(client));
                        
                        // Запускаем поток клиента и сохраняем его для отслеживания
                        {
                            std::lock_guard<std::mutex> thread_lock(m_client_threads_mutex);
                            m_client_threads.emplace_back(&WebSocketServer::handleClient, this, client_fd);
                        }
                    } else {
                        LOG_ERROR("Не удалось инициализировать wslay контекст");
                        close(client_fd);
                    }
                } else {
                    close(client_fd);
                }
            }
        }
    }
    
    close(server_fd);
}

bool WebSocketServer::performHandshake(int client_fd) {
    char buffer[1024];
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        LOG_ERROR("Не удалось прочитать данные handshake");
        return false;
    }
    
    buffer[bytes_read] = '\0';
    std::string request(buffer);
    
    std::string key;
    size_t key_pos = request.find("Sec-WebSocket-Key:");
    if (key_pos != std::string::npos) {
        size_t start = key_pos + 18;
        size_t end = request.find("\r\n", start);
        if (end != std::string::npos) {
            key = request.substr(start, end - start);
            size_t first = key.find_first_not_of(" \t");
            size_t last = key.find_last_not_of(" \t");
            if (first != std::string::npos && last != std::string::npos) {
                key = key.substr(first, last - first + 1);
            }
        }
    }
    
    if (key.empty()) {
        LOG_ERROR("Sec-WebSocket-Key не найден");
        return false;
    }
    
    std::string accept_key = sha1_hash(key);
    
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << accept_key << "\r\n"
             << "Sec-WebSocket-Protocol: json\r\n"
             << "\r\n";
    
    std::string response_str = response.str();
    if (write(client_fd, response_str.c_str(), response_str.size()) < 0) {
        LOG_ERROR("Не удалось отправить handshake ответ");
        return false;
    }
    
    LOG_INFO("WebSocket handshake успешен");
    return true;
}

ssize_t WebSocketServer::recv_callback(wslay_event_context* ctx, uint8_t* data, size_t len, int flags, void* user_data) {
    WslayClient* client = static_cast<WslayClient*>(user_data);
    if (!client || client->fd == -1) {
        return -1;
    }
    
    ssize_t ret = recv(client->fd, data, len, MSG_DONTWAIT);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }
        return -1;
    }
    
    return ret;
}

ssize_t WebSocketServer::send_callback(wslay_event_context* ctx, const uint8_t* data, size_t len, int flags, void* user_data) {
    WslayClient* client = static_cast<WslayClient*>(user_data);
    if (!client || client->fd == -1) {
        return -1;
    }
    
    ssize_t ret = send(client->fd, data, len, MSG_NOSIGNAL);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }
        return -1;
    }
    
    return ret;
}

void WebSocketServer::on_msg_recv_callback(wslay_event_context* ctx, const wslay_event_on_msg_recv_arg* msg, void* user_data) {
    WslayClient* client = static_cast<WslayClient*>(user_data);
    if (!client || !client->ctx || !client->server) {
        return;
    }
    
    WebSocketServer* server = client->server;
    
    if (!server->checkRateLimit(client->fd)) {
        return;
    }
    
    if (msg->opcode == WSLAY_TEXT_FRAME) {
        std::string message(reinterpret_cast<const char*>(msg->msg), msg->msg_length);
        
        try {
            json j = json::parse(message);
            
            if (j.contains("type")) {
                std::string type = j["type"];
                
                if (type == "ping") {
                    json pong_response;
                    pong_response["type"] = "pong";
                    pong_response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    // Отправляем pong напрямую через send
                    struct wslay_event_msg pong_msg;
                    pong_msg.opcode = WSLAY_TEXT_FRAME;
                    pong_msg.msg = reinterpret_cast<const uint8_t*>(pong_response.dump().c_str());
                    pong_msg.msg_length = pong_response.dump().size();
                    wslay_event_queue_msg(ctx, &pong_msg);
                    wslay_event_send(ctx);
                }
                // Обработка команд управления от веб-джойстиков
                else if (type == "command" && server->m_command_callback) {
                    Command cmd{};
                    
                    if (j.contains("left_y")) cmd.left_y = j["left_y"].get<float>();
                    if (j.contains("right_y")) cmd.right_y = j["right_y"].get<float>();
                    if (j.contains("left_x")) cmd.left_x = j["left_x"].get<float>();
                    if (j.contains("right_x")) cmd.right_x = j["right_x"].get<float>();
                    if (j.contains("tower_h")) cmd.tower_h = j["tower_h"].get<int>();
                    if (j.contains("fire")) cmd.fire = j["fire"].get<bool>();
                    if (j.contains("lights")) cmd.lights = j["lights"].get<bool>();
                    if (j.contains("pointer")) cmd.pointer = j["pointer"].get<bool>();
                    
                    server->m_command_callback(cmd);
                }
                // Обработка команд автономности
                else if ((type == "autonomy" || type == "auto_mode") && server->m_autonomy_callback) {
                    server->m_autonomy_callback(message);
                }
            }
        } catch (const std::exception& e) {
            LOG_WARNING("Ошибка парсинга JSON команды: {}", e.what());
        }
    }
}

void WebSocketServer::sendString(WslayClient* client, const std::string& str) {
    if (!client || !client->ctx || client->fd == -1) {
        return;
    }
    
    struct wslay_event_msg msg;
    msg.opcode = WSLAY_TEXT_FRAME;
    msg.msg = reinterpret_cast<const uint8_t*>(str.c_str());
    msg.msg_length = str.size();
    
    if (wslay_event_queue_msg(client->ctx, &msg) != 0) {
        LOG_WARNING("Не удалось поставить сообщение в очередь для клиента");
    }
    
    wslay_event_send(client->ctx);
}

void WebSocketServer::handleClient(int client_fd) {
    WslayClient* client = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find_if(m_clients.begin(), m_clients.end(),
            [client_fd](const std::unique_ptr<WslayClient>& c) {
                return c && c->fd == client_fd;
            });
        if (it != m_clients.end()) {
            client = it->get();
        }
    }
    
    if (!client) {
        close(client_fd);
        // Очищаем поток из списка при ошибке
        std::lock_guard<std::mutex> thread_lock(m_client_threads_mutex);
        auto it = std::find_if(m_client_threads.begin(), m_client_threads.end(),
            [](std::thread& t) { return t.joinable(); });
        if (it != m_client_threads.end()) {
            it->join();
            m_client_threads.erase(it);
        }
        return;
    }
    
    while (m_running.load()) {
        uint8_t buffer[4096];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), MSG_DONTWAIT);
        
        if (bytes_read > 0) {
            if (wslay_event_recv(client->ctx) != 0) {
                break;
            }
        } else if (bytes_read == 0 || (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            break;
        }
        
        if (wslay_event_send(client->ctx) != 0) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find_if(m_clients.begin(), m_clients.end(),
            [client_fd](const std::unique_ptr<WslayClient>& c) {
                return c && c->fd == client_fd;
            });
        if (it != m_clients.end()) {
            m_clients.erase(it);
        }
    }
    
    cleanupClientRateLimit(client_fd);
    
    close(client_fd);
    LOG_INFO("Клиент WebSocket отключился");
}

void WebSocketServer::setCommandCallback(std::function<void(const Command&)> callback) {
    m_command_callback = callback;
}

void WebSocketServer::setAutonomyCallback(std::function<void(const std::string&)> callback) {
    m_autonomy_callback = callback;
}

void WebSocketServer::broadcastTelemetry(const Telemetry& telemetry, float cpu_temp, float memory_percent,
                                        float heading, float mag_x, float mag_y, float mag_z,
                                        float ultrasonic_dist, int wifi_quality) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_clients.empty()) {
        LOG_DEBUG("WebSocket: нет подключенных клиентов, телеметрия не отправляется");
        return;
    }
    
    LOG_DEBUG("WebSocket: отправка телеметрии {} клиентам", m_clients.size());
    
    json j;
    j["type"] = "telemetry";
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Основная телеметрия
    j["telemetry"]["battery_voltage"] = telemetry.battery_voltage;
    j["telemetry"]["roll"] = telemetry.roll;
    j["telemetry"]["pitch"] = telemetry.pitch;
    j["telemetry"]["yaw"] = telemetry.yaw;
    j["telemetry"]["turret_angle"] = telemetry.turret_angle;
    j["telemetry"]["current_left"] = telemetry.current_left;
    j["telemetry"]["current_right"] = telemetry.current_right;
    j["telemetry"]["current_tower"] = telemetry.current_tower;
    j["telemetry"]["signal_quality"] = telemetry.signal_quality;
    j["telemetry"]["arduino_online"] = telemetry.arduino_online;
    j["telemetry"]["gyro_ready"] = telemetry.gyro_ready;
    
    // Системная информация
    j["system"]["cpu_temp"] = cpu_temp;
    j["system"]["memory_percent"] = memory_percent;
    j["system"]["wifi_quality"] = wifi_quality;
    
    // Датчики
    if (heading >= 0) {
        j["sensors"]["heading"] = heading;
        j["sensors"]["compass_valid"] = true;
    } else {
        j["sensors"]["compass_valid"] = false;
    }
    
    j["sensors"]["mag_x"] = mag_x;
    j["sensors"]["mag_y"] = mag_y;
    j["sensors"]["mag_z"] = mag_z;
    
    if (ultrasonic_dist >= 0) {
        j["sensors"]["ultrasonic_distance"] = ultrasonic_dist;
        j["sensors"]["ultrasonic_valid"] = true;
    } else {
        j["sensors"]["ultrasonic_valid"] = false;
    }
    
    std::string message = j.dump();
    
    std::vector<WslayClient*> clients_to_send;
    for (auto& client : m_clients) {
        if (client && client->ctx) {
            clients_to_send.push_back(client.get());
        }
    }
    
    for (auto* client : clients_to_send) {
        sendString(client, message);
    }
}

} // namespace robo_chassis
