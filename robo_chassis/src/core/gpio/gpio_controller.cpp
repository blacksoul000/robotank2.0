#include "gpio_controller.hpp"
#include "i_imu.hpp"
#include "mpu6050_imu.hpp"
#include "complementary_filter.hpp"
#include "logger/logger.hpp"

#include <cstring>
#include <cmath>
#include <map>
#include <climits>

// pigpio headers (если доступен)
#ifdef HAVE_PIGPIO
// Пробуем pigpiod_if2.h для клиентской библиотеки
#  ifdef __has_include
#    if __has_include(<pigpiod_if2.h>)
#      include <pigpiod_if2.h>
#    elif __has_include(<pigpio.h>)
#      include <pigpio.h>
#    else
#      undef HAVE_PIGPIO
#    endif
#  else
#    include <pigpiod_if2.h>
#  endif
#endif

namespace robo_chassis {

namespace {
    // Пины GPIO
    const uint8_t SHOT_PIN = 27;
    const uint8_t SHOT_FINISHED_PIN1 = 20;
    const uint8_t SHOT_FINISHED_PIN2 = 21;
    const uint8_t RESET_PIN = 4;
    const uint8_t POINTER_PIN = 26;
    const uint8_t GUN_V_PIN = 13;

    const int TICK_INTERVAL = 10; // мс
    constexpr double GUN_TICK_COEF = 60.0 / SHRT_MAX;
    constexpr double POSITION_COEF = 360.0 / SHRT_MAX;

    // Биты кнопок джойстика
    constexpr uint16_t TRIGGER_LEFT_BIT = 6;
    constexpr uint16_t TRIGGER_RIGHT_BIT = 7;
    constexpr uint16_t POINTER_BIT = 5;
}

class GpioController::Impl {
public:
    struct ServoInfo {
        uint16_t minPulse = 0;
        uint16_t maxPulse = 0;
        uint16_t pulse = 0;
        uint16_t realPulse = 0;
        int8_t tick = 0;
        uint16_t zeroLift = 0;
        float pulsePerDegree = 0.0f;
    };

    struct ImuData {
        std::unique_ptr<IImu> imu;
        bool ready = false;
        bool online = false;
        float yawOffset = 0.0f;
    };

    // Callback'и
    JoyCallback joyCb;
    InfluenceCallback influenceCb;
    DeviationCallback deviationCb;
    EmptyCallback gunCalibrateCb;
    EmptyCallback cameraCalibrateCb;
    EmptyCallback gyroCalibrateCb;
    StatusCallback statusCb;

    GunPositionCallback gunPositionCb;
    PointerCallback pointerCb;
    YprCallback yprCb;

    // IMU данные
    ImuData chassis;
    ImuData tower;

    // Состояние
    bool shooting = false;
    bool shotClosing = false;
    bool pointer = false;

    double towerH = 0.0;
    PointF3D chassisGyroData;

    // Сервоприводы
    std::map<uint8_t, ServoInfo> servo;

    // Кнопки джойстика
    uint16_t buttons = 0;

    void onShotStatusChanged(bool shot);
    void onPointerTriggered();

    template<class T>
    bool isBitSet(T value, uint8_t bit) const {
        return (value & (1 << bit));
    }

    bool gpioInitialized = false;
};

void GpioController::Impl::onShotStatusChanged(bool shot) {
    LOG_INFO_SRC("Shot status: " + std::string(shot ? "ON" : "OFF"), "gpio_controller");
#ifdef HAVE_PIGPIO
    gpioWrite(SHOT_FINISHED_PIN1, shot);
    gpioWrite(SHOT_PIN, shot);
#endif
    shooting = shot;
}

void GpioController::Impl::onPointerTriggered() {
    pointer = !pointer;
    LOG_INFO_SRC("Pointer: " + std::string(pointer ? "ON" : "OFF"), "gpio_controller");
#ifdef HAVE_PIGPIO
    gpioWrite(POINTER_PIN, pointer);
#endif
    if (pointerCb) {
        pointerCb(pointer);
    }
}

GpioController::GpioController() : d(std::make_unique<Impl>()) {
    // Инициализация сервопривода оружия
    d->servo[GUN_V_PIN] = {1200, 1670, 1600, 0, 0, 1600, 33.3201f};
}

GpioController::~GpioController() {
#ifdef HAVE_PIGPIO
    if (d->gpioInitialized) {
        gpioWrite(SHOT_FINISHED_PIN1, 0);
        gpioTerminate();
    }
#endif
}

void GpioController::start() {
    // Инициализация гироскопов
    d->tower.imu = std::make_unique<Mpu6050Imu>("/dev/i2c-1", 0x69, 0.0f);
    d->chassis.imu = std::make_unique<Mpu6050Imu>("/dev/i2c-1", 0x68, 0.0f);

    for (auto& imuData : {&d->chassis, &d->tower}) {
        if (imuData->imu && imuData->imu->init()) {
            imuData->online = true;
            if (d->statusCb) {
                d->statusCb(true);
            }
        } else {
            LOG_ERROR_SRC("Failed to init IMU", "gpio_controller");
        }
    }

    // Инициализация GPIO
#ifdef HAVE_PIGPIO
    if (gpioInitialise() >= 0) {
        d->gpioInitialized = true;
        
        // Установка таймера для сервоприводов
        gpioSetTimerFuncEx(0, TICK_INTERVAL, servoTickProxy, this);

        LOG_INFO_SRC("GPIO initialized", "gpio_controller");
    } else {
        LOG_ERROR_SRC("Failed to initialize GPIO", "gpio_controller");
    }
#else
    LOG_INFO_SRC("pigpio not available, running in simulation mode", "gpio_controller");
#endif
}

void GpioController::execute() {
    // Проверка статуса выстрела
    if (d->shooting) {
#ifdef HAVE_PIGPIO
        if (gpioRead(SHOT_FINISHED_PIN2)) {
            d->shotClosing = true;
        } else if (d->shotClosing) {
            d->shotClosing = false;
            d->onShotStatusChanged(false);
        }
#else
        // В режиме симуляции просто выключаем
        if (d->shotClosing) {
            d->shotClosing = false;
            d->onShotStatusChanged(false);
        }
#endif
    }

    // Чтение данных гироскопов
    for (auto& imuData : {&d->chassis, &d->tower}) {
        if (imuData->imu) {
            imuData->imu->readData();

            if (imuData->imu->isReady() != imuData->ready) {
                imuData->ready = imuData->imu->isReady();
                imuData->yawOffset = imuData->imu->yaw();
            }
        }
    }

    // Вычисление положения оружия
    float towerH = (d->chassis.ready && d->tower.ready && d->chassis.imu && d->tower.imu)
                   ? static_cast<float>(d->towerH - d->tower.yawOffset)
                   : 0.0f;

    float gunY = 0.0f;
    auto it = d->servo.find(GUN_V_PIN);
    if (it != d->servo.end() && it->second.pulsePerDegree > 0) {
        gunY = static_cast<float>((it->second.maxPulse - it->second.pulse) / it->second.pulsePerDegree);
    }

    // Публикация положения оружия
    if (d->gunPositionCb) {
        d->gunPositionCb(towerH, gunY);
    }

    // Публикация данных ориентации
    if (d->chassis.ready && d->chassis.imu) {
        PointF3D ypr;
        ypr.x = d->chassis.imu->yaw() - d->chassis.yawOffset;
        ypr.y = d->chassis.imu->roll();
        ypr.z = d->chassis.imu->pitch();

        if (d->yprCb) {
            d->yprCb(ypr);
        }
    }

    // Вычисление угла башни
    if (d->chassis.imu && d->tower.imu) {
        d->towerH = d->tower.imu->yaw() - d->chassis.imu->yaw();
    }
}

void GpioController::setJoyCallback(JoyCallback cb) {
    d->joyCb = std::move(cb);
}

void GpioController::setInfluenceCallback(InfluenceCallback cb) {
    d->influenceCb = std::move(cb);
}

void GpioController::setDeviationCallback(DeviationCallback cb) {
    d->deviationCb = std::move(cb);
}

void GpioController::setGunCalibrateCallback(EmptyCallback cb) {
    d->gunCalibrateCb = std::move(cb);
}

void GpioController::setCameraCalibrateCallback(EmptyCallback cb) {
    d->cameraCalibrateCb = std::move(cb);
}

void GpioController::setGyroCalibrateCallback(EmptyCallback cb) {
    d->gyroCalibrateCb = std::move(cb);
}

void GpioController::setStatusCallback(StatusCallback cb) {
    d->statusCb = std::move(cb);
}

void GpioController::setGunPositionCallback(GunPositionCallback cb) {
    d->gunPositionCb = std::move(cb);
}

void GpioController::setPointerCallback(PointerCallback cb) {
    d->pointerCb = std::move(cb);
}

void GpioController::setYprCallback(YprCallback cb) {
    d->yprCb = std::move(cb);
}

void GpioController::onJoyEvent(uint16_t buttons) {
    if (d->buttons == buttons) return;

    bool triggersPressed = d->isBitSet(buttons, TRIGGER_LEFT_BIT) && 
                           d->isBitSet(buttons, TRIGGER_RIGHT_BIT);
    bool pointerPressed = d->isBitSet(buttons, POINTER_BIT);

    bool prevTriggersPressed = d->isBitSet(d->buttons, TRIGGER_LEFT_BIT) && 
                               d->isBitSet(d->buttons, TRIGGER_RIGHT_BIT);

    if (triggersPressed != prevTriggersPressed) {
        if (triggersPressed) {
            d->onShotStatusChanged(!d->shooting);
        }
    }

    if (pointerPressed != d->isBitSet(d->buttons, POINTER_BIT)) {
        if (pointerPressed) {
            d->onPointerTriggered();
        }
    }

    d->buttons = buttons;

    if (d->joyCb) {
        d->joyCb({buttons});
    }
}

void GpioController::onInfluence(const Influence& influence) {
    auto it = d->servo.find(GUN_V_PIN);
    if (it != d->servo.end()) {
        it->second.tick = static_cast<int8_t>(std::ceil(influence.gunV * GUN_TICK_COEF));
    }

    if (d->influenceCb) {
        d->influenceCb(influence);
    }
}

void GpioController::onDeviation(double value) {
    auto it = d->servo.find(GUN_V_PIN);
    if (it != d->servo.end()) {
        it->second.tick = 0;
        it->second.pulse = static_cast<uint16_t>(
            it->second.maxPulse - value * it->second.pulsePerDegree
        );
    }

    if (d->deviationCb) {
        d->deviationCb(value);
    }
}

void GpioController::onGunCalibrate() {
    auto it = d->servo.find(GUN_V_PIN);
    if (it != d->servo.end()) {
        it->second.zeroLift = it->second.pulse;
    }
    d->tower.yawOffset = static_cast<float>(d->towerH);

    if (d->gunCalibrateCb) {
        d->gunCalibrateCb({});
    }
}

void GpioController::onCameraCalibrate() {
    if (d->cameraCalibrateCb) {
        d->cameraCalibrateCb({});
    }
}

void GpioController::onGyroCalibrate() {
    if (d->chassis.imu) {
        d->chassis.yawOffset = d->chassis.imu->yaw();
    }

    if (d->gyroCalibrateCb) {
        d->gyroCalibrateCb({});
    }
}

void GpioController::onArduinoStatusChanged(bool online) {
    if (d->statusCb) {
        d->statusCb(online);
    }
}

void GpioController::servoTickProxy(void* data) {
    reinterpret_cast<GpioController*>(data)->servoTick();
}

void GpioController::servoTick() {
    for (auto& [pin, info] : d->servo) {
        info.pulse = static_cast<uint16_t>(
            std::max(static_cast<int>(info.minPulse),
                     std::min(static_cast<int>(info.maxPulse),
                              static_cast<int>(info.pulse) + info.tick))
        );

#ifdef HAVE_PIGPIO
        gpioServo(pin, info.pulse);
#endif
    }
}

} // namespace robo_chassis
