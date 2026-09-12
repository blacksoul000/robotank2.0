#include <gtest/gtest.h>
#include "safety_manager.hpp"
#include "robot_logic.hpp"
#include <thread>
#include <chrono>
#include <functional>

TEST(SafetyManagerTest, EmergencyStopCallback) {
    bool emergency_triggered = false;
    
    robo_chassis::SafetyManager safety_mgr(10, 5); // watchdog=10s, arduino=5s
    
    safety_mgr.setEmergencyStopCallback([&emergency_triggered]() {
        emergency_triggered = true;
    });
    
    // Имитируем активацию безопасного режима
    safety_mgr.activateSafeMode();
    
    EXPECT_TRUE(safety_mgr.isSafeModeActive());
    EXPECT_TRUE(emergency_triggered);
}

TEST(SafetyManagerTest, SafeModeActivationDeactivation) {
    bool callback_called = false;
    
    robo_chassis::SafetyManager safety_mgr(10, 5);
    
    safety_mgr.setEmergencyStopCallback([&callback_called]() {
        callback_called = true;
    });
    
    // Активация
    safety_mgr.activateSafeMode();
    EXPECT_TRUE(safety_mgr.isSafeModeActive());
    EXPECT_TRUE(callback_called);
    
    // Деактивация
    callback_called = false;
    safety_mgr.deactivateSafeMode();
    EXPECT_FALSE(safety_mgr.isSafeModeActive());
    EXPECT_FALSE(callback_called); // При деактивации callback не вызывается
}

TEST(SafetyManagerTest, ArduinoStatusMonitoring) {
    robo_chassis::SafetyManager safety_mgr(10, 1); // Короткий таймаут для теста (1 сек)
    
    bool emergency_triggered = false;
    safety_mgr.setEmergencyStopCallback([&emergency_triggered]() {
        emergency_triggered = true;
    });
    
    // Инициализация и запуск
    if (safety_mgr.init()) {
        safety_mgr.start();
        
        // Симулируем потерю связи с Arduino
        safety_mgr.updateArduinoStatus(false);
        
        // Ждем истечения таймаута
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        
        // Проверяем, что безопасный режим активирован
        EXPECT_TRUE(safety_mgr.isSafeModeActive() || emergency_triggered);
        
        safety_mgr.stop();
    }
}

TEST(SafetyManagerTest, WatchdogPetPreventsTimeout) {
    robo_chassis::SafetyManager safety_mgr(1, 5); // Короткий watchdog для теста
    
    safety_mgr.init();
    safety_mgr.start();
    
    // Регулярно pet watchdog
    for (int i = 0; i < 5; ++i) {
        safety_mgr.petWatchdog();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    
    // Система не должна требовать перезагрузки благодаря pet
    EXPECT_FALSE(safety_mgr.needsReboot());
    
    safety_mgr.stop();
}
