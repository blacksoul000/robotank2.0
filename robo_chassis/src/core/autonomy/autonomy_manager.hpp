#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include "sensors/compass_ultrasonic.hpp"
#include "sensors/sensor_fusion.hpp"

namespace robo_chassis {

// Состояния автомата автономности
enum class AutoState {
    IDLE,              // Ожидание команды
    HOLD_HEADING,      // Удержание курса
    AVOID_OBSTACLE,    // Обход препятствия
    PATROL             // Патрулирование (будущее расширение)
};

// Команды для шасси
struct ChassisCommand {
    float linear = 0.0f;   // -1.0 .. 1.0 (назад/вперед)
    float angular = 0.0f;  // -1.0 .. 1.0 (поворот влево/вправо)
    bool stop_requested = false;
};

// Контекст для принятия решений
struct AutonomyContext {
    float current_heading = 0.0f;    // Текущий курс (0-360)
    float target_heading = 0.0f;     // Целевой курс
    float ultrasonic_dist = 0.0f;    // Дистанция до препятствия (см)
    bool mag_available = false;      // Доступен ли магнитометр
    bool imu_available = false;      // Доступен ли IMU
};

// PID регулятор для плавного поворота
class PIDController {
public:
    PIDController(float kp = 1.5f, float ki = 0.0f, float kd = 0.5f);
    
    void setTarget(float target);
    void setCurrent(float current);
    float compute();
    void reset();
    
    void setGains(float kp, float ki, float kd);
    
private:
    float kp_, ki_, kd_;
    float target_ = 0.0f;
    float current_ = 0.0f;
    float integral_ = 0.0f;
    float prev_error_ = 0.0f;
    uint64_t last_time_ms_ = 0;
    
    float normalizeAngle(float angle);
    float calculateError(float target, float current);
};

// Менеджер автономности
class AutonomyManager {
public:
    using CommandCallback = std::function<void(const ChassisCommand&)>;
    
    explicit AutonomyManager(CommandCallback cmd_callback);
    ~AutonomyManager() = default;
    
    // Инициализация
    bool init(sensors::Compass* compass, sensors::Ultrasonic* ultrasonic, SensorFusion* fusion);
    
    // Основной цикл (вызывать в цикле управления)
    void update();
    
    // Управление режимами
    void setState(AutoState state);
    AutoState getState() const { return state_; }
    
    // Установка целевого курса (для режима HOLD_HEADING)
    void setTargetHeading(float heading_deg);
    float getTargetHeading() const { return target_heading_; }
    
    // Настройки
    void setObstacleThreshold(float cm) { obstacle_threshold_cm_ = cm; }
    void setTurnAngle(float deg) { avoid_turn_angle_deg_ = deg; }
    void setAvoidanceDuration(float sec) { avoid_duration_sec_ = sec; }
    
    // Статус
    std::string getStateString() const;
    bool isObstacleDetected() const { return obstacle_detected_; }
    
private:
    CommandCallback cmd_callback_;
    
    // Датчики
    sensors::Compass* compass_ = nullptr;
    sensors::Ultrasonic* ultrasonic_ = nullptr;
    SensorFusion* fusion_ = nullptr;
    
    // Состояние
    AutoState state_ = AutoState::IDLE;
    float target_heading_ = 0.0f;
    bool obstacle_detected_ = false;
    
    // Тайминги для обхода препятствий
    uint64_t avoid_start_time_ms_ = 0;
    float avoid_turn_direction_ = 1.0f; // 1.0 = вправо, -1.0 = влево
    
    // Настройки
    float obstacle_threshold_cm_ = 25.0f;  // Дистанция срабатывания (см)
    float avoid_turn_angle_deg_ = 90.0f;   // Угол поворота при обходе
    float avoid_duration_sec_ = 2.0f;      // Время движения после поворота
    
    // PID контроллер для удержания курса
    PIDController heading_pid_;
    
    // Внутренние методы
    ChassisCommand handleIdle();
    ChassisCommand handleHoldHeading(const AutonomyContext& ctx);
    ChassisCommand handleAvoidObstacle(const AutonomyContext& ctx);
    ChassisCommand handlePatrol(const AutonomyContext& ctx);
    
    void sendCommand(const ChassisCommand& cmd);
    void sendStop();
    
    uint64_t getCurrentTimeMs() const;
};

} // namespace robo_chassis
