#ifndef GPIO_CONTROLLER_HPP
#define GPIO_CONTROLLER_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <atomic>

namespace robo_chassis {

// Структуры для данных
struct JoyButtons {
    uint16_t buttons = 0;
};

struct Influence {
    float gunV = 0.0f;
};

struct Empty {};

struct PointF3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

/**
 * @brief Контроллер GPIO для управления периферией робота
 * 
 * Управляет сервоприводами, считывает данные гироскопов,
 * обрабатывает команды стрельбы и указателя.
 */
class GpioController {
public:
    using JoyCallback = std::function<void(const JoyButtons&)>;
    using InfluenceCallback = std::function<void(const Influence&)>;
    using DeviationCallback = std::function<void(double)>;
    using EmptyCallback = std::function<void(const Empty&)>;
    using StatusCallback = std::function<void(bool)>;
    using GunPositionCallback = std::function<void(float, float)>; // x, y
    using PointerCallback = std::function<void(bool)>;
    using YprCallback = std::function<void(const PointF3D&)>;

    GpioController();
    ~GpioController();

    /**
     * @brief Запуск контроллера
     */
    void start();

    /**
     * @brief Основной цикл обработки
     */
    void execute();

    // Callback'и для установки обработчиков событий
    void setJoyCallback(JoyCallback cb);
    void setInfluenceCallback(InfluenceCallback cb);
    void setDeviationCallback(DeviationCallback cb);
    void setGunCalibrateCallback(EmptyCallback cb);
    void setCameraCalibrateCallback(EmptyCallback cb);
    void setGyroCalibrateCallback(EmptyCallback cb);
    void setStatusCallback(StatusCallback cb);

    // Callback'и для публикации данных
    void setGunPositionCallback(GunPositionCallback cb);
    void setPointerCallback(PointerCallback cb);
    void setYprCallback(YprCallback cb);

    /**
     * @brief Обработать событие кнопок джойстика
     */
    void onJoyEvent(uint16_t buttons);

    /**
     * @brief Обработать управляющее воздействие
     */
    void onInfluence(const Influence& influence);

    /**
     * @brief Обратить отклонение (для автокалибровки оружия)
     */
    void onDeviation(double value);

    /**
     * @brief Калибровка оружия
     */
    void onGunCalibrate();

    /**
     * @brief Калибровка камеры
     */
    void onCameraCalibrate();

    /**
     * @brief Калибровка гироскопа
     */
    void onGyroCalibrate();

    /**
     * @brief Обработка изменения статуса Arduino
     */
    void onArduinoStatusChanged(bool online);

private:
    class Impl;
    std::unique_ptr<Impl> d;

    void servoTick();
    static void servoTickProxy(void* data);
};

} // namespace robo_chassis

#endif // GPIO_CONTROLLER_HPP
