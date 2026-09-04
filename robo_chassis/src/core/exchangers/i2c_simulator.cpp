#include "i2c_simulator.hpp"
#include <iostream>
#include <cstring>

namespace robo_chassis {

class I2CSimulator::Impl {
public:
    size_t package_size = 9;
    int read_interval_ms = 100;
    
    std::thread read_thread;
    std::atomic<bool> running{false};
    
    DataCallback data_callback;
    
    // Состояние симуляции
    float heading = 0.0f;          // Курс в градусах (0-360)
    float distance_front = 100.0f; // Дистанция переднего датчика в см
    float distance_left = 100.0f;  // Дистанция левого датчика в см
    float distance_right = 100.0f; // Дистанция правого датчика в см
    uint8_t motor_state = 0;       // Состояние моторов
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> noise_dist;
    
    Impl() : rng(std::random_device{}()), noise_dist(-2.0f, 2.0f) {}
    
    std::vector<uint8_t> generate_sensor_data();
};

std::vector<uint8_t> I2CSimulator::Impl::generate_sensor_data() {
    std::vector<uint8_t> data(package_size, 0);
    
    // Эмуляция постепенного изменения курса (как от компаса)
    heading += noise_dist(rng) * 0.5f;
    if (heading < 0.0f) heading += 360.0f;
    if (heading >= 360.0f) heading -= 360.0f;
    
    // Эмуляция препятствий (периодически уменьшаем дистанцию)
    static int obstacle_timer = 0;
    obstacle_timer++;
    if (obstacle_timer > 50) { // Каждые ~5 секунд при 100ms интервале
        distance_front = 20.0f + noise_dist(rng) * 5.0f; // Препятствие спереди
        if (obstacle_timer > 100) {
            obstacle_timer = 0;
        }
    } else {
        distance_front = 100.0f + noise_dist(rng) * 5.0f; // Свободно
    }
    
    distance_left = 80.0f + noise_dist(rng) * 10.0f;
    distance_right = 80.0f + noise_dist(rng) * 10.0f;
    
    // Формат пакета данных (9 байт):
    // [0-1]: курс (uint16_t, 0-3600 = 0-360.0 градусов)
    // [2-3]: дистанция спереди (uint16_t, см)
    // [4-5]: дистанция слева (uint16_t, см)
    // [6-7]: дистанция справа (uint16_t, см)
    // [8]: состояние моторов (uint8_t)
    
    uint16_t heading_raw = static_cast<uint16_t>(heading * 10.0f);
    uint16_t front_raw = static_cast<uint16_t>(distance_front);
    uint16_t left_raw = static_cast<uint16_t>(distance_left);
    uint16_t right_raw = static_cast<uint16_t>(distance_right);
    
    data[0] = (heading_raw >> 8) & 0xFF;
    data[1] = heading_raw & 0xFF;
    
    data[2] = (front_raw >> 8) & 0xFF;
    data[3] = front_raw & 0xFF;
    
    data[4] = (left_raw >> 8) & 0xFF;
    data[5] = left_raw & 0xFF;
    
    data[6] = (right_raw >> 8) & 0xFF;
    data[7] = right_raw & 0xFF;
    
    data[8] = motor_state;
    
    return data;
}

I2CSimulator::I2CSimulator(size_t package_size, int read_interval_ms) 
    : d(new Impl) {
    d->package_size = package_size;
    d->read_interval_ms = read_interval_ms;
}

I2CSimulator::~I2CSimulator() {
    close();
    delete d;
}

bool I2CSimulator::open() {
    if (d->running) {
        return true; // Already open
    }
    
    d->running = true;
    d->read_thread = std::thread(&I2CSimulator::read_loop, this);
    
    std::cout << "[I2CSimulator] Simulation mode started\n";
    std::cout << "  Package size: " << d->package_size << " bytes\n";
    std::cout << "  Update interval: " << d->read_interval_ms << " ms\n";
    
    return true;
}

void I2CSimulator::close() {
    if (!d->running) {
        return;
    }
    
    d->running = false;
    
    if (d->read_thread.joinable()) {
        d->read_thread.join();
    }
    
    std::cout << "[I2CSimulator] Simulation stopped\n";
}

bool I2CSimulator::is_open() const {
    return d->running;
}

void I2CSimulator::set_data_callback(DataCallback callback) {
    d->data_callback = std::move(callback);
}

bool I2CSimulator::send_data(const uint8_t* data, size_t len) {
    if (!d->running) {
        return false;
    }
    
    // В режиме симуляции просто логируем отправленные данные
    // Можно добавить обработку команд для изменения состояния симуляции
    if (len >= 1) {
        std::cout << "[I2CSimulator] Received command: 0x" 
                  << std::hex << static_cast<int>(data[0]) << std::dec << "\n";
    }
    
    return true;
}

void I2CSimulator::read_loop() {
    while (d->running) {
        auto data = d->generate_sensor_data();
        
        if (d->data_callback) {
            d->data_callback(data);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(d->read_interval_ms));
    }
}

} // namespace robo_chassis
