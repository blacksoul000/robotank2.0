#include "robot_logic.hpp"
#include "exchangers/i_exchanger.hpp"
#include "gpio/mpu6050_imu.hpp"
#include <iostream>
#include <chrono>
#include <cstring>

// Конструктор по умолчанию для внутренней инициализации
RobotLogic::RobotLogic() {
    m_out_package.powerDown = 0;
    m_out_package.light = 0;
    m_out_package.reserve = 0;
    m_out_package.leftEngine = 0;
    m_out_package.rightEngine = 0;
    m_out_package.towerH = 0;
    m_out_package.crc = 0;
    
    m_offsets.voltage = 0;
    m_offsets.currentLeft = 0;
    m_offsets.currentRight = 0;
    m_offsets.currentTower = 0;
    m_offsets.crc = 0;
    
    m_simulation_mode = false;
}

RobotLogic::RobotLogic(std::unique_ptr<robo_chassis::IExchanger> exchanger, bool simulation_mode)
    : RobotLogic() {
    m_exchanger = std::move(exchanger);
    m_simulation_mode = simulation_mode;
    
    if (m_exchanger) {
        m_exchanger->set_data_callback(
            [this](const std::vector<uint8_t>& data) {
                on_arduino_data_received(data);
            }
        );
    }
    
    if (m_simulation_mode) {
        std::cout << "[RobotLogic] Инициализирован в режиме СИМУЛЯЦИИ\n";
    } else {
        std::cout << "RobotLogic initialized with IExchanger.\n";
    }
}

RobotLogic::~RobotLogic() {
    if (m_exchanger) {
        m_exchanger->close();
    }
    std::cout << "RobotLogic destroyed.\n";
}

void RobotLogic::init_imu(const std::string& i2c_device) {
    std::cout << "[RobotLogic] Инициализация IMU...\n";
    
    // Создаем IMU для шасси (адрес 0x68)
    m_chassis_imu = std::make_unique<robo_chassis::Mpu6050Imu>(i2c_device, 0x68);
    if (m_chassis_imu->init()) {
        std::cout << "[RobotLogic] IMU шасси инициализировано\n";
    } else {
        std::cerr << "[RobotLogic] Не удалось инициализировать IMU шасси\n";
        m_chassis_imu.reset();
    }
    
    // Создаем IMU для башни (адрес 0x69)
    m_tower_imu = std::make_unique<robo_chassis::Mpu6050Imu>(i2c_device, 0x69);
    if (m_tower_imu->init()) {
        std::cout << "[RobotLogic] IMU башни инициализировано\n";
    } else {
        std::cerr << "[RobotLogic] Не удалось инициализировать IMU башни\n";
        m_tower_imu.reset();
    }
}

void RobotLogic::process_command(const Command& cmd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_current_cmd = cmd;
    
    // Преобразование команд в управляющие воздействия для моторов
    m_out_package.leftEngine = static_cast<int16_t>(cmd.left_y * 32767.0f);
    m_out_package.rightEngine = static_cast<int16_t>(cmd.right_y * 32767.0f);
    m_out_package.towerH = cmd.tower_h;
    m_out_package.light = cmd.lights ? 1 : 0;
    
    // Обработка команды стрельбы (логика из GpioController)
    if (cmd.fire && !m_shooting) {
        m_shooting = true;
        std::cout << "ОГОНЬ!\n";
    } else if (!cmd.fire && m_shooting) {
        m_shot_closing = true;
    }
}

Telemetry RobotLogic::get_telemetry() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_telemetry;
}

void RobotLogic::calibrate_gyro() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Калибровка гироскопа шасси
    if (m_chassis_imu && m_chassis_imu->isReady()) {
        m_chassis_yaw_offset = m_chassis_imu->yaw();
        m_chassis_imu->setYawOffset(0.0f);
        std::cout << "Гироскоп шасси откалиброван.\n";
    }
    
    // Калибровка гироскопа башни
    if (m_tower_imu && m_tower_imu->isReady()) {
        m_tower_yaw_offset = m_tower_imu->yaw();
        m_tower_imu->setYawOffset(0.0f);
        std::cout << "Гироскоп башни откалиброван.\n";
    }
    
    m_telemetry.yaw = 0.0f;
}

void RobotLogic::calibrate_gun() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Установка нулевого угла возвышения оружия
    std::cout << "Оружие откалибровано.\n";
}

uint16_t RobotLogic::crc16(const unsigned char* data, unsigned short len) const {
    unsigned short crc = 0xFFFF;
    unsigned char i;

    while (len--) {
        crc ^= *data++ << 8;

        for (i = 0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

bool RobotLogic::validate_arduino_package(const ArduinoPkg* pkg) const {
    return pkg->crc == crc16(reinterpret_cast<const unsigned char*>(pkg), 
                              sizeof(ArduinoPkg) - sizeof(pkg->crc));
}

void RobotLogic::process_arduino_data(const uint8_t* data, size_t len) {
    if (len != sizeof(ArduinoPkg)) {
        // В режиме симуляции I2CSimulator отправляет 9 байт, а не sizeof(ArduinoPkg)
        if (!m_simulation_mode) {
            std::cerr << "Неверный размер пакета от Arduino: " << len << "\n";
        }
        return;
    }
    
    const ArduinoPkg* pkg = reinterpret_cast<const ArduinoPkg*>(data);
    
    if (!validate_arduino_package(pkg)) {
        if (!m_simulation_mode) {
            std::cerr << "Ошибка CRC в пакете от Arduino\n";
        }
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Обновление времени последнего ответа
    m_last_arduino_response = std::chrono::steady_clock::now();
    
    // Проверка онлайн статуса Arduino
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_arduino_response).count();
    m_telemetry.arduino_online = (elapsed < ARDUINO_TIMEOUT_MS);
    
    // Применение смещений и обновление телеметрии
    m_telemetry.battery_voltage = static_cast<float>(pkg->voltage - m_offsets.voltage) / 100.0f;
    m_telemetry.current_left = pkg->currentLeft - m_offsets.currentLeft;
    m_telemetry.current_right = pkg->currentRight - m_offsets.currentRight;
    m_telemetry.current_tower = pkg->currentTower - m_offsets.currentTower;
}

void RobotLogic::read_sensors() {
    // Чтение данных с IMU шасси
    if (m_chassis_imu) {
        m_chassis_imu->readData();
        
        if (m_chassis_imu->isReady()) {
            m_telemetry.roll = m_chassis_imu->roll();
            m_telemetry.pitch = m_chassis_imu->pitch();
            m_telemetry.yaw = m_chassis_imu->yaw() - m_chassis_yaw_offset;
            m_telemetry.gyro_ready = true;
        }
    }
    
    // Чтение данных с IMU башни и вычисление угла башни относительно шасси
    if (m_tower_imu && m_chassis_imu && 
        m_tower_imu->isReady() && m_chassis_imu->isReady()) {
        
        m_tower_imu->readData();
        
        float tower_yaw = m_tower_imu->yaw() - m_tower_yaw_offset;
        float chassis_yaw = m_chassis_imu->yaw() - m_chassis_yaw_offset;
        
        // Угол башни относительно шасси
        m_telemetry.turret_angle = tower_yaw - chassis_yaw;
        
        // Нормализация угла в диапазон [-180, 180]
        while (m_telemetry.turret_angle > 180.0f) {
            m_telemetry.turret_angle -= 360.0f;
        }
        while (m_telemetry.turret_angle < -180.0f) {
            m_telemetry.turret_angle += 360.0f;
        }
    }
}

void RobotLogic::on_arduino_data_received(const std::vector<uint8_t>& data) {
    if (data.size() != sizeof(ArduinoPkg)) {
        std::cerr << "[RobotLogic] Неверный размер пакета от Arduino: " 
                  << data.size() << " байт вместо " << sizeof(ArduinoPkg) << "\n";
        return;
    }
    
    process_arduino_data(data.data(), data.size());
}

void RobotLogic::update_telemetry() {
    // Данные от Arduino приходят через callback в on_arduino_data_received
    
    // Чтение данных с гироскопов (MPU6050 через I2C)
    read_sensors();

    telemetry_updated = true;
}

void RobotLogic::send_to_arduino() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Вычисление CRC
    m_out_package.crc = crc16(reinterpret_cast<unsigned char*>(&m_out_package),
                               sizeof(RaspberryPkg) - sizeof(m_out_package.crc));

    // Отправка через IExchanger
    if (m_exchanger && m_exchanger->is_open()) {
        m_exchanger->send_data(reinterpret_cast<uint8_t*>(&m_out_package), sizeof(m_out_package));
    }
}

