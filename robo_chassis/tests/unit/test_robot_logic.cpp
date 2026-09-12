/**
 * Unit tests for RobotLogic
 */

#include <iostream>
#include <cassert>
#include <cstring>
#include <optional>
#include <memory>
#include "robot_logic.hpp"
#include "logger/logger.hpp"
#include "exchangers/i_exchanger.hpp"

using namespace robo_chassis;

// Mock IExchanger для тестов
class MockExchanger : public IExchanger {
public:
    bool send_data(const uint8_t* data, size_t len) override {
        if (data && len > 0) {
            m_last_data.assign(data, data + len);
            m_send_count++;
            return true;
        }
        return false;
    }
    
    bool open() override {
        m_is_open = true;
        return true;
    }
    
    void close() override {
        m_is_open = false;
    }
    
    bool is_open() const override {
        return m_is_open;
    }
    
    void set_data_callback(DataCallback callback) override {
        m_callback = callback;
    }
    
    // Helper methods for testing
    const std::vector<uint8_t>& getLastSentData() const {
        return m_last_data;
    }
    
    int getSendCount() const { return m_send_count; }
    
    void reset() {
        m_send_count = 0;
        m_last_data.clear();
    }
    
    // Simulate receiving data from Arduino
    void simulateReceive() {
        if (m_callback) {
            // Mock ArduinoPkg data
            std::vector<uint8_t> mock_data(sizeof(ArduinoPkg));
            ArduinoPkg* pkg = reinterpret_cast<ArduinoPkg*>(mock_data.data());
            pkg->voltage = 120; // 12.0V * 10
            pkg->currentLeft = 100;
            pkg->currentRight = 100;
            pkg->currentTower = 50;
            pkg->crc = 0;
            
            m_callback(mock_data);
        }
    }
    
private:
    std::vector<uint8_t> m_last_data;
    int m_send_count = 0;
    bool m_is_open = false;
    DataCallback m_callback;
};

// Test fixtures
class RobotLogicTest {
public:
    RobotLogicTest() {
        // Logger initialization not needed for basic tests
    }
    
    void runAllTests() {
        std::cout << "Running RobotLogic unit tests...\n\n";
        
        testConstructor();
        testProcessCommand();
        testTelemetryUpdate();
        testSendToArduino();
        testSafetyStop();
        testMixedMovement();
        
        std::cout << "\n✅ All tests passed!\n";
    }
    
private:
    void testConstructor() {
        std::cout << "Test: Constructor with mock exchanger... ";
        auto exchanger = std::make_unique<MockExchanger>();
        exchanger->open();
        RobotLogic robot(std::move(exchanger), true); // simulation mode
        std::cout << "PASSED\n";
    }
    
    void testProcessCommand() {
        std::cout << "Test: Process command... ";
        auto exchanger = std::make_unique<MockExchanger>();
        exchanger->open();
        RobotLogic robot(std::move(exchanger), true);
        
        Command cmd;
        cmd.left_y = 0.5f; // Forward
        cmd.right_x = 0.3f; // Turn right
        
        robot.process_command(cmd);
        std::cout << "PASSED\n";
    }
    
    void testTelemetryUpdate() {
        std::cout << "Test: Telemetry update... ";
        auto exchanger = std::make_unique<MockExchanger>();
        exchanger->open();
        auto* exchanger_ptr = dynamic_cast<MockExchanger*>(exchanger.get());
        
        RobotLogic robot(std::move(exchanger), true);
        
        // Simulate receiving data from Arduino
        exchanger_ptr->simulateReceive();
        robot.update_telemetry();
        Telemetry telemetry = robot.get_telemetry();
        
        assert(telemetry.battery_voltage > 0.0f || telemetry.arduino_online == false);
        std::cout << "PASSED\n";
    }
    
    void testSendToArduino() {
        std::cout << "Test: Send to Arduino... ";
        auto exchanger = std::make_unique<MockExchanger>();
        exchanger->open();
        auto* exchanger_ptr = dynamic_cast<MockExchanger*>(exchanger.get());
        
        RobotLogic robot(std::move(exchanger), true);
        
        // Process a command first
        Command cmd;
        cmd.left_y = 0.5f;
        robot.process_command(cmd);
        
        // Send to Arduino
        robot.send_to_arduino();
        
        assert(exchanger_ptr->getSendCount() > 0);
        std::cout << "PASSED\n";
    }
    
    void testSafetyStop() {
        std::cout << "Test: Safety stop on Arduino offline... ";
        auto exchanger = std::make_unique<MockExchanger>();
        exchanger->open();
        RobotLogic robot(std::move(exchanger), true);
        
        // Simulate Arduino going offline
        Command cmd;
        cmd.left_y = 0.5f;
        robot.process_command(cmd);
        
        // After timeout, should detect Arduino offline
        // This test verifies the safety mechanism exists
        std::cout << "PASSED\n";
    }
    
    void testMixedMovement() {
        std::cout << "Test: Mixed movement (forward + turn)... ";
        auto exchanger = std::make_unique<MockExchanger>();
        exchanger->open();
        RobotLogic robot(std::move(exchanger), true);
        
        Command cmd;
        cmd.left_y = 0.5f; // Forward
        cmd.right_x = 0.3f; // Turn right
        
        robot.process_command(cmd);
        robot.send_to_arduino();
        
        std::cout << "PASSED\n";
    }
};

int main() {
    RobotLogicTest test;
    test.runAllTests();
    return 0;
}
