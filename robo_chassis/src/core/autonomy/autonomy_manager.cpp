#include "autonomy_manager.hpp"
#include "logger/logger.hpp"
#include "sensors/compass_ultrasonic.hpp"
#include <chrono>
#include <cmath>

namespace robo_chassis {

using namespace sensors;

// ============================================================================
// PIDController
// ============================================================================

PIDController::PIDController(float kp, float ki, float kd)
    : kp_(kp), ki_(ki), kd_(kd) {
    last_time_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void PIDController::setTarget(float target) {
    target_ = normalizeAngle(target);
}

void PIDController::setCurrent(float current) {
    current_ = normalizeAngle(current);
}

float PIDController::normalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float PIDController::calculateError(float target, float current) {
    float error = target - current;
    // Нормализация ошибки для кратчайшего пути
    if (error > 180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;
    return error;
}

float PIDController::compute() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    float dt = static_cast<float>(now - last_time_ms_) / 1000.0f;
    
    if (dt <= 0.0f || dt > 1.0f) {
        dt = 0.016f; // Защита от деления на ноль и больших скачков (60 FPS)
    }
    
    float error = calculateError(target_, current_);
    
    // Пропорциональная составляющая
    float p_term = kp_ * error;
    
    // Интегральная составляющая (с ограничением)
    integral_ += error * dt;
    if (integral_ > 100.0f) integral_ = 100.0f;
    if (integral_ < -100.0f) integral_ = -100.0f;
    float i_term = ki_ * integral_;
    
    // Дифференциальная составляющая
    float derivative = (error - prev_error_) / dt;
    float d_term = kd_ * derivative;
    
    prev_error_ = error;
    last_time_ms_ = now;
    
    float output = p_term + i_term + d_term;
    
    // Ограничение выхода (-1.0 .. 1.0)
    if (output > 1.0f) output = 1.0f;
    if (output < -1.0f) output = -1.0f;
    
    return output;
}

void PIDController::reset() {
    integral_ = 0.0f;
    prev_error_ = 0.0f;
    last_time_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void PIDController::setGains(float kp, float ki, float kd) {
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
}

// ============================================================================
// AutonomyManager
// ============================================================================

AutonomyManager::AutonomyManager(CommandCallback cmd_callback)
    : cmd_callback_(std::move(cmd_callback))
    , heading_pid_(1.5f, 0.02f, 0.5f) {  // Настройки PID по умолчанию
}

bool AutonomyManager::init(Compass* compass, Ultrasonic* ultrasonic, SensorFusion* fusion) {
    compass_ = compass;
    ultrasonic_ = ultrasonic;
    fusion_ = fusion;
    
    LOG_INFO("AutonomyManager initialized");
    LOG_INFO("  Compass: {}", compass_ ? "OK" : "NOT FOUND");
    LOG_INFO("  Ultrasonic: {}", ultrasonic_ ? "OK" : "NOT FOUND");
    LOG_INFO("  SensorFusion: {}", fusion_ ? "OK" : "NOT FOUND");
    
    return true;
}

void AutonomyManager::update() {
    AutonomyContext ctx;
    
    // Сбор данных с датчиков
    if (fusion_ && fusion_->isInitialized()) {
        ctx.current_heading = fusion_->getHeading();
        ctx.imu_available = true;
        ctx.mag_available = fusion_->isMagnetometerAvailable();
    } else if (compass_ && compass_->isReady()) {
        ctx.current_heading = compass_->getHeading();
        ctx.mag_available = true;
        ctx.imu_available = false;
    } else {
        ctx.current_heading = 0.0f;
        ctx.mag_available = false;
        ctx.imu_available = false;
    }
    
    // Чтение ультразвука
    if (ultrasonic_ && ultrasonic_->isReady()) {
        ctx.ultrasonic_dist = ultrasonic_->readDistanceCm();
    } else {
        ctx.ultrasonic_dist = 999.0f; // Нет препятствий
    }
    
    // Проверка препятствий
    obstacle_detected_ = (ctx.ultrasonic_dist < obstacle_threshold_cm_);
    
    // Автоматический переход в режим обхода при препятствии
    if (obstacle_detected_ && state_ != AutoState::AVOID_OBSTACLE) {
        LOG_WARNING("Obstacle detected at {:.1f} cm! Switching to AVOID_OBSTACLE", 
                    ctx.ultrasonic_dist);
        setState(AutoState::AVOID_OBSTACLE);
    }
    
    // Выполнение логики текущего состояния
    ChassisCommand cmd;
    switch (state_) {
        case AutoState::IDLE:
            cmd = handleIdle();
            break;
        case AutoState::HOLD_HEADING:
            cmd = handleHoldHeading(ctx);
            break;
        case AutoState::AVOID_OBSTACLE:
            cmd = handleAvoidObstacle(ctx);
            break;
        case AutoState::PATROL:
            cmd = handlePatrol(ctx);
            break;
    }
    
    sendCommand(cmd);
}

void AutonomyManager::setState(AutoState state) {
    if (state_ == state) return;
    
    LOG_INFO("Autonomy state changed: {} -> {}", getStateString(), 
             state == AutoState::IDLE ? "IDLE" :
             state == AutoState::HOLD_HEADING ? "HOLD_HEADING" :
             state == AutoState::AVOID_OBSTACLE ? "AVOID_OBSTACLE" : "PATROL");
    
    state_ = state;
    
    // Сброс PID при смене состояния
    if (state == AutoState::HOLD_HEADING) {
        heading_pid_.reset();
        heading_pid_.setTarget(target_heading_);
    }
}

void AutonomyManager::setTargetHeading(float heading_deg) {
    target_heading_ = heading_deg;
    if (heading_deg > 360.0f) target_heading_ = 360.0f;
    if (heading_deg < 0.0f) target_heading_ = 0.0f;
    
    if (state_ == AutoState::HOLD_HEADING) {
        heading_pid_.setTarget(target_heading_);
    }
}

std::string AutonomyManager::getStateString() const {
    switch (state_) {
        case AutoState::IDLE: return "IDLE";
        case AutoState::HOLD_HEADING: return "HOLD_HEADING";
        case AutoState::AVOID_OBSTACLE: return "AVOID_OBSTACLE";
        case AutoState::PATROL: return "PATROL";
        default: return "UNKNOWN";
    }
}

ChassisCommand AutonomyManager::handleIdle() {
    return ChassisCommand{0.0f, 0.0f, false};
}

ChassisCommand AutonomyManager::handleHoldHeading(const AutonomyContext& ctx) {
    // Если нет данных о курсе, останавливаемся
    if (!ctx.mag_available && !ctx.imu_available) {
        LOG_WARNING("No heading data available for HOLD_HEADING mode");
        return ChassisCommand{0.0f, 0.0f, true};
    }
    
    // Обновление PID контроллера
    heading_pid_.setCurrent(ctx.current_heading);
    float angular_output = heading_pid_.compute();
    
    // Небольшое движение вперед для инерции (опционально)
    float linear = 0.2f; // Медленное движение вперед
    
    LOG_DEBUG("HOLD_HEADING: target={:.1f}, current={:.1f}, output={:.2f}",
              target_heading_, ctx.current_heading, angular_output);
    
    return ChassisCommand{linear, angular_output, false};
}

ChassisCommand AutonomyManager::handleAvoidObstacle(const AutonomyContext& ctx) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    // Этап 1: Поворот на 90 градусов
    if (now - avoid_start_time_ms_ < 1500) { // 1.5 секунды на поворот
        float turn_speed = avoid_turn_direction_ * 0.6f;
        LOG_DEBUG("AVOID: Turning (direction={})", avoid_turn_direction_);
        return ChassisCommand{0.0f, turn_speed, false};
    }
    // Этап 2: Движение вперед для объезда
    else if (now - avoid_start_time_ms_ < 1500 + static_cast<uint64_t>(avoid_duration_sec_ * 1000)) {
        LOG_DEBUG("AVOID: Moving forward to bypass");
        return ChassisCommand{0.4f, 0.0f, false};
    }
    // Этап 3: Возврат к удержанию курса или ожидание
    else {
        LOG_INFO("Obstacle avoidance complete");
        
        // Если препятствие всё ещё близко, повторяем манёвр в другую сторону
        if (ctx.ultrasonic_dist < obstacle_threshold_cm_) {
            LOG_WARNING("Obstacle still detected, trying opposite direction");
            avoid_turn_direction_ *= -1.0f;
            avoid_start_time_ms_ = now;
            return ChassisCommand{0.0f, avoid_turn_direction_ * 0.6f, false};
        }
        
        // Возврат в режим удержания курса или IDLE
        setState(AutoState::HOLD_HEADING);
        return ChassisCommand{0.0f, 0.0f, false};
    }
}

ChassisCommand AutonomyManager::handlePatrol(const AutonomyContext& ctx) {
    // Заглушка для будущего режима патрулирования
    // Будет реализовано в следующей итерации
    LOG_DEBUG("PATROL mode not yet implemented");
    return handleIdle();
}

void AutonomyManager::sendCommand(const ChassisCommand& cmd) {
    if (cmd_callback_) {
        cmd_callback_(cmd);
    }
}

void AutonomyManager::sendStop() {
    sendCommand(ChassisCommand{0.0f, 0.0f, true});
}

} // namespace robo_chassis
