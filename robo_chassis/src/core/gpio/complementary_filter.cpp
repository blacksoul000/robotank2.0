#include "complementary_filter.hpp"

#include <cmath>

namespace robo_chassis {

namespace {
    constexpr float FILTER_CONSTANT = 0.40f; // Коэффициент комплементарного фильтра
}

class ComplementaryFilter::Impl {
public:
    // Данные акселерометра
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;

    // Данные гироскопа
    float gx = 0.0f;
    float gy = 0.0f;
    float gz = 0.0f;

    // Данные магнитометра
    float mx = 0.0f;
    float my = 0.0f;
    float mz = 0.0f;

    // Вычисленные углы
    float pitch = 0.0f;
    float roll = 0.0f;
    float yaw = 0.0f;

    bool useMag = true;
};

ComplementaryFilter::ComplementaryFilter() : d(new Impl()) {
    reset();
}

ComplementaryFilter::~ComplementaryFilter() {
    delete d;
}

void ComplementaryFilter::setUseMag(bool use) {
    d->useMag = use;
}

void ComplementaryFilter::setGyroData(float gx, float gy, float gz) {
    d->gx = gx;
    d->gy = gy;
    d->gz = gz;
}

void ComplementaryFilter::setAccelData(float ax, float ay, float az) {
    d->ax = ax;
    d->ay = ay;
    d->az = az;
}

void ComplementaryFilter::setMagData(float mx, float my, float mz) {
    d->mx = mx;
    d->my = my;
    d->mz = mz;
}

void ComplementaryFilter::process(float dt) {
    // Преобразование значений акселерометра в углы (в градусах)
    float accelXAngle = std::atan2(d->ay, d->az) * 180.0f / static_cast<float>(M_PI);
    float accelYAngle = std::atan2(d->ax, std::sqrt(d->ay * d->ay + d->az * d->az)) *
                        180.0f / static_cast<float>(M_PI);

    // Комплементарный фильтр для объединения данных акселерометра и гироскопа
    d->pitch = FILTER_CONSTANT * (d->pitch + d->gy * dt) +
               (1.0f - FILTER_CONSTANT) * accelYAngle;
    d->roll = FILTER_CONSTANT * (d->roll + d->gx * dt) +
              (1.0f - FILTER_CONSTANT) * accelXAngle;

    // Нормализация сырых значений акселерометра
    float l = std::sqrt(d->ax * d->ax + d->ay * d->ay + d->az * d->az);
    float accXnorm = d->ax / l;
    float accYnorm = d->ay / l;

    // Вычисление pitch и roll для компенсированного yaw
    float magPitch = std::asin(accXnorm);
    float magRoll = -std::asin(accYnorm / std::cos(magPitch));

    if (d->useMag) {
        // Вычисление компенсированных по наклону значений магнитометра
        float magXcomp = d->mx * std::cos(magPitch) + d->mz * std::sin(magPitch);
        float magYcomp = d->mx * std::sin(magRoll) * std::sin(magPitch) +
                         d->my * std::cos(magRoll) -
                         d->mz * std::sin(magRoll) * std::cos(magPitch);
        d->yaw = 180.0f * std::atan2(magYcomp, magXcomp) / static_cast<float>(M_PI);
    } else {
        d->yaw = 180.0f * std::atan(d->az / std::sqrt(d->ax * d->ax + d->az * d->az)) /
                 static_cast<float>(M_PI);
    }

    // Нормализация угла yaw в диапазон [0, 360)
    if (d->yaw < 0.0f) {
        d->yaw += 360.0f;
    }
}

void ComplementaryFilter::reset() {
    d->ax = 0.0f;
    d->ay = 0.0f;
    d->az = 0.0f;
    d->gx = 0.0f;
    d->gy = 0.0f;
    d->gz = 0.0f;
    d->mx = 0.0f;
    d->my = 0.0f;
    d->mz = 0.0f;
    d->pitch = 0.0f;
    d->roll = 0.0f;
    d->yaw = 0.0f;
}

float ComplementaryFilter::pitch() const {
    return d->pitch;
}

float ComplementaryFilter::roll() const {
    return d->roll;
}

float ComplementaryFilter::yaw() const {
    return d->yaw;
}

} // namespace robo_chassis
