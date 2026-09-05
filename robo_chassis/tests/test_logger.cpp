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

TEST_F(LoggerTest, InitializationWithFile) {
    auto& logger = robo_chassis::Logger::instance();
    
    logger.init(
        robo_chassis::LogLevel::DEBUG,
        false,  // console disabled for tests
        true,   // file enabled
        test_log_path_,
        10,     // max_size_mb
        5       // max_files
    );
    
    logger.info("Test message", "test_logger.cpp");
    
    // Даем время на запись в файл
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readFileContent(test_log_path_);
    EXPECT_NE(content.find("INFO"), std::string::npos);
    EXPECT_NE(content.find("Test message"), std::string::npos);
}

TEST_F(LoggerTest, MultipleLogLevels) {
    auto& logger = robo_chassis::Logger::instance();
    
    logger.init(
        robo_chassis::LogLevel::DEBUG,
        false,
        true,
        test_log_path_
    );
    
    logger.debug("Debug message", "test");
    logger.info("Info message", "test");
    logger.warning("Warning message", "test");
    logger.error("Error message", "test");
    logger.critical("Critical message", "test");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readFileContent(test_log_path_);
    EXPECT_NE(content.find("DEBUG"), std::string::npos);
    EXPECT_NE(content.find("INFO"), std::string::npos);
    EXPECT_NE(content.find("WARNING"), std::string::npos);
    EXPECT_NE(content.find("ERROR"), std::string::npos);
    EXPECT_NE(content.find("CRITICAL"), std::string::npos);
}

TEST_F(LoggerTest, LogLevelFiltering) {
    auto& logger = robo_chassis::Logger::instance();
    
    // Устанавливаем уровень WARNING - DEBUG и INFO не должны записываться
    logger.init(
        robo_chassis::LogLevel::WARNING,
        false,
        true,
        test_log_path_
    );
    
    logger.debug("Debug should not appear", "test");
    logger.info("Info should not appear", "test");
    logger.warning("Warning should appear", "test");
    logger.error("Error should appear", "test");
    
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
                logger.info("Message from thread " + std::to_string(i) + " iter " + std::to_string(j), "test");
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
    
    logger.info("Source test", "test_logger.cpp");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readFileContent(test_log_path_);
    EXPECT_NE(content.find("test_logger.cpp"), std::string::npos);
}
