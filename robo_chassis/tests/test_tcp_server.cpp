#include <gtest/gtest.h>
#include "tcp_server.hpp"
#include "robot_logic.hpp"
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <memory>
#include <vector>
#include <functional>

// Mock RobotLogic для тестов
class MockRobotLogic : public RobotLogic {
public:
    MockRobotLogic() : RobotLogic(nullptr, nullptr) {}
    
    void executeCommand(const Command& cmd) override {
        last_command = cmd;
    }
    
    Command last_command;
};

class TcpServerTest : public ::testing::Test {
protected:
    std::unique_ptr<TcpServer> server;
    std::unique_ptr<MockRobotLogic> mock_robot;
    const int test_port = 18766; // Нестандартный порт для тестов
    
    void SetUp() override {
        mock_robot = std::make_unique<MockRobotLogic>();
        server = std::make_unique<TcpServer>(test_port, *mock_robot);
    }
    
    void TearDown() override {
        if (server) {
            server->stop();
        }
    }
};

TEST_F(TcpServerTest, StartAndStop) {
    // Запускаем сервер в отдельном потоке
    std::thread server_thread([this]() {
        server->run();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server->stop();
    
    if (server_thread.joinable()) {
        server_thread.join();
    }
    // Если не упало - тест пройден
}

TEST_F(TcpServerTest, SingleClientConnection) {
    std::thread server_thread([this]() {
        server->run();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Создаем клиентский сокет
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GT(client_fd, 0);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(test_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Подключаемся
    int result = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    ASSERT_EQ(result, 0);
    
    // Закрываем клиент
    close(client_fd);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server->stop();
    
    if (server_thread.joinable()) {
        server_thread.join();
    }
}

TEST_F(TcpServerTest, SendMessageToServer) {
    std::thread server_thread([this]() {
        server->run();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GT(client_fd, 0);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(test_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    int result = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    ASSERT_EQ(result, 0);
    
    // Отправляем тестовое сообщение
    const char* msg = "{\"type\":\"test\"}\n";
    ssize_t sent = send(client_fd, msg, strlen(msg), 0);
    EXPECT_GT(sent, 0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    close(client_fd);
    server->stop();
    
    if (server_thread.joinable()) {
        server_thread.join();
    }
}
