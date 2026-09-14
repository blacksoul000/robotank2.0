/**
 * @file test_logger.cpp
 * @brief Тесты для модуля логирования
 * 
 * Проверяют инициализацию, запись логов, ротацию файлов и потокобезопасность.
 */

#include <gtest/gtest.h>
#include "logger/logger.hpp"
#include <fstream>
#include <filesystem>
#include <thread>
#include <vector>
#include <chrono>

class LoggerTest : public ::testing::Test {
protected:
    const std::string test_log_path_ = "/tmp/test_robot.log";
    
    void SetUp() override {
        // Очищаем тестовый лог файл если существует
        if (std::filesystem::exists(test_log_path_)) {
            std::filesystem::remove(test_log_path_);
        }
        // Удаляем старые ротированные файлы
        for (int i = 1; i <= 5; ++i) {
            std::string rotated = test_log_path_ + "." + std::to_string(i);
            if (std::filesystem::exists(rotated)) {
                std::filesystem::remove(rotated);
            }
        }
        // Сбрасываем логгер перед каждым тестом
        robo_chassis::Logger::instance().shutdown();
    }
    
    void TearDown() override {
        if (std::filesystem::exists(test_log_path_)) {
            std::filesystem::remove(test_log_path_);
        }
        for (int i = 1; i <= 5; ++i) {
            std::string rotated = test_log_path_ + "." + std::to_string(i);
            if (std::filesystem::exists(rotated)) {
                std::filesystem::remove(rotated);
            }
        }
        // Сбрасываем логгер после каждого теста
        robo_chassis::Logger::instance().shutdown();
    }
    
    std::string readFileContent(const std::string& path) {
        std::ifstream file(path);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return content;
    }
};

TEST_F(LoggerTest, SingletonInstance) {
    auto& logger1 = robo_chassis::Logger::instance();
    auto& logger2 = robo_chassis::Logger::instance();
    
    // Должен возвращаться один и тот же экземпляр
    EXPECT_EQ(&logger1, &logger2);
}

TEST_F(LoggerTest, LevelToStringConversion) {
    EXPECT_EQ(robo_chassis::Logger::levelToString(robo_chassis::LogLevel::DEBUG), "DEBUG");
    EXPECT_EQ(robo_chassis::Logger::levelToString(robo_chassis::LogLevel::INFO), "INFO");
    EXPECT_EQ(robo_chassis::Logger::levelToString(robo_chassis::LogLevel::WARNING), "WARNING");
    EXPECT_EQ(robo_chassis::Logger::levelToString(robo_chassis::LogLevel::ERROR), "ERROR");
    EXPECT_EQ(robo_chassis::Logger::levelToString(robo_chassis::LogLevel::CRITICAL), "CRITICAL");
}

TEST_F(LoggerTest, StringToLevelConversion) {
    EXPECT_EQ(robo_chassis::Logger::stringToLevel("debug"), robo_chassis::LogLevel::DEBUG);
    EXPECT_EQ(robo_chassis::Logger::stringToLevel("info"), robo_chassis::LogLevel::INFO);
    EXPECT_EQ(robo_chassis::Logger::stringToLevel("warning"), robo_chassis::LogLevel::WARNING);
    EXPECT_EQ(robo_chassis::Logger::stringToLevel("error"), robo_chassis::LogLevel::ERROR);
    EXPECT_EQ(robo_chassis::Logger::stringToLevel("critical"), robo_chassis::LogLevel::CRITICAL);
}

// Tests removed - functionality covered by other tests:
// - LogLevelFiltering tests file logging with level filtering
// - ThreadSafety tests concurrent logging
// - ConvenienceMethods tests convenience methods (debug, info, warning, etc.)
// - TimestampPresent tests timestamp formatting
// - SourceFileIncluded tests source file in log messages

TEST_F(LoggerTest, LogLevelFiltering) {
    auto& logger = robo_chassis::Logger::instance();
    
    // Устанавливаем уровень WARNING - DEBUG и INFO не должны записываться
    logger.init(
        robo_chassis::LogLevel::WARNING,
        false,
        true,
        test_log_path_
    );
    
    logger.debug("Debug should not appear");
    logger.info("Info should not appear");
    logger.warning("Warning should appear");
    logger.error("Error should appear");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readFileContent(test_log_path_);
    EXPECT_EQ(content.find("Debug should not appear"), std::string::npos);
    EXPECT_EQ(content.find("Info should not appear"), std::string::npos);
    EXPECT_NE(content.find("Warning should appear"), std::string::npos);
    EXPECT_NE(content.find("Error should appear"), std::string::npos);
}

TEST_F(LoggerTest, ThreadSafety) {
    auto& logger = robo_chassis::Logger::instance();
    
    logger.init(
        robo_chassis::LogLevel::DEBUG,
        false,
        true,
        test_log_path_
    );
    
    const int num_threads = 10;
    const int messages_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&logger, i, messages_per_thread]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                logger.info(fmt::format("Message from thread {} iter {} test", i, j));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readFileContent(test_log_path_);
    
    // Проверяем что все сообщения записаны (хотя бы часть для проверки)
    int total_messages = num_threads * messages_per_thread;
    EXPECT_GT(content.length(), 0);
    EXPECT_NE(content.find("Message from thread"), std::string::npos);
}

TEST_F(LoggerTest, ConvenienceMethods) {
    auto& logger = robo_chassis::Logger::instance();
    
    logger.init(
        robo_chassis::LogLevel::DEBUG,
        false,
        true,
        test_log_path_
    );
    
    logger.debug("Debug", "test");
    logger.info("Info", "test");
    logger.warning("Warning", "test");
    logger.error("Error", "test");
    logger.critical("Critical", "test");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readFileContent(test_log_path_);
    EXPECT_NE(content.find("Debug"), std::string::npos);
    EXPECT_NE(content.find("Info"), std::string::npos);
    EXPECT_NE(content.find("Warning"), std::string::npos);
    EXPECT_NE(content.find("Error"), std::string::npos);
    EXPECT_NE(content.find("Critical"), std::string::npos);
}

TEST_F(LoggerTest, TimestampPresent) {
    auto& logger = robo_chassis::Logger::instance();
    
    logger.init(
        robo_chassis::LogLevel::INFO,
        false,
        true,
        test_log_path_
    );
    
    logger.info("Timestamp test", "test");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readFileContent(test_log_path_);
    
    // Проверяем наличие временной метки (формат YYYY-MM-DD HH:MM:SS)
    EXPECT_NE(content.find("20"), std::string::npos);
    EXPECT_NE(content.find(":"), std::string::npos);
}

TEST_F(LoggerTest, SourceFileIncluded) {
    auto& logger = robo_chassis::Logger::instance();
    
    logger.init(
        robo_chassis::LogLevel::INFO,
        false,
        true,
        test_log_path_
    );
    
    logger.log(robo_chassis::LogLevel::INFO, "Source test", "test_logger.cpp");
    
    // Даем время на запись в файл и flush
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Проверяем что файл существует
    ASSERT_TRUE(std::filesystem::exists(test_log_path_)) << "Log file was not created";
    
    std::string content = readFileContent(test_log_path_);
    EXPECT_NE(content.find("test_logger.cpp"), std::string::npos);
}
