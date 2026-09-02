# RoboChassis - Система управления роботом-танком

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%202-green.svg)](https://www.raspberrypi.org/)
[![License](https://img.shields.io/badge/license-MIT-yellow.svg)](LICENSE)

Современная система управления роботом на базе Raspberry Pi 2 Model B с управлением через веб-интерфейс на телефоне.

## 📋 Возможности

- ✅ **Веб-интерфейс** - управление с любого устройства через браузер
- ✅ **Два сенсорных джойстика** - раздельное управление движением и башней
- ✅ **WebRTC видеотрансляция** - минимальная задержка (~100-300 мс)
- ✅ **Телеметрия в реальном времени**:
  - 🔋 Заряд батареи (V, %)
  - 📐 Крен и тангаж (MPU6050)
  - 🎯 Угол башни
  - ⚡ Ток моторов
- ✅ **Низкое потребление памяти** - ~5 МБ для C++ ядра
- ✅ **Аппаратное кодирование H.264** через VPU Raspberry Pi

## 🏗 Архитектура

```
Телефон (браузер)
    ↓ Wi-Fi (HTTP/WebSocket)
Python Bridge (aiohttp + websockets)
    ↓ TCP (localhost:5555)
C++ Ядро (robo_chassis)
    ↓ I²C/UART
Arduino → Моторы, сервы, датчики
```

**Видеопоток:** Отдельный процесс `webrtc-streamer` передаёт видео с камеры через WebRTC напрямую в браузер.

Подробная документация архитектуры: [ARCHITECTURE.md](ARCHITECTURE.md)

## 📁 Структура проекта

```
robotank/
├── README.md                 # Этот файл
├── ARCHITECTURE.md           # Подробная архитектура системы
├── robo_chassis/             # C++ ядро системы управления
│   ├── src/core/             # Исходный код C++20
│   │   ├── main.cpp          # Точка входа
│   │   ├── tcp_server.*      # TCP сервер (порт 5555)
│   │   ├── serial_port.*     # UART общение с Arduino
│   │   ├── robot_logic.*     # Логика управления
│   │   ├── exchangers/       # Обмен данными (I2C, UART)
│   │   └── gpio/             # GPIO, MPU6050, фильтры
│   ├── python_bridge/        # Python мост к браузеру
│   ├── static/               # Веб-интерфейс (HTML/CSS/JS)
│   ├── CMakeLists.txt        # Сборка C++
│   └── start_robot.sh        # Скрипт запуска
├── python_bridge/            # Python мост (дубликат для удобства)
│   ├── bridge.py             # Основной код моста
│   └── requirements.txt      # Python зависимости
└── robo_arduino/             # Прошивка Arduino
    ├── robo_arduino.ino      # Основной скетч
    ├── I2Cdev.*              # I2C библиотека
    ├── MPU6050.*             # Драйвер MPU6050
    └── LowPower.*            # Библиотека энергосбережения
```

## 🚀 Быстрый старт

### 1. Подготовка Raspberry Pi

```bash
# Включить камеру и I2C
sudo raspi-config
# Enable Camera и I2C в Interface Options

# Обновить систему
sudo apt update && sudo apt upgrade -y

# Установить зависимости
sudo apt install -y \
    cmake g++ make \
    libjsoncpp-dev \
    python3 python3-pip \
    webrtc-streamer \
    libcamera-dev v4l-utils

# Установить Python зависимости
pip3 install aiohttp websockets

# Добавить пользователя в группы
sudo usermod -aG video,dialout $USER
```

### 2. Сборка C++ ядра

```bash
cd robo_chassis
mkdir -p build && cd build
cmake ..
make -j4
```

### 3. Запуск

```bash
# Автоматический запуск всех компонентов
./start_robot.sh
```

Или ручной запуск по отдельности:

```bash
# 1. C++ ядро
./build/robo_chassis &

# 2. Python Bridge
python3 python_bridge/bridge.py &

# 3. WebRTC стример
webrtc-streamer -H 0.0.0.0:8000 rpi:///dev/video0 &
```

### 4. Подключение

1. Узнайте IP адрес Raspberry Pi: `hostname -I`
2. Откройте в браузере телефона: `http://<IP_RPI>:8080`
3. Разрешите доступ к полноэкранному режиму
4. Управляйте роботом с помощью джойстиков

## 📡 Порты и протоколы

| Сервис | Порт | Протокол | Назначение |
|--------|------|----------|------------|
| Python Bridge HTTP | 8080 | HTTP | Веб-интерфейс |
| Python Bridge WebSocket | 8765 | WebSocket | Обмен данными с браузером |
| C++ TCP Server | 5555 | TCP | Связь Python ↔ C++ |
| WebRTC Streamer HTTP | 8000 | HTTP | Видеопоток WebRTC |
| WebRTC Streamer RTSP | 8554 | RTSP | Внутренний поток |

## 📦 Форматы сообщений

### Команда управления (Browser → C++)

```json
{
  "type": "COMMAND",
  "left_x": 0.0,
  "left_y": -0.5,
  "right_x": 0.3,
  "right_y": 0.0,
  "fire": false,
  "lights": true
}
```

### Телеметрия (C++ → Browser)

```json
{
  "type": "TELEMETRY",
  "battery": 12.4,
  "roll": 2.5,
  "pitch": -1.3,
  "turret_angle": 45,
  "signal_quality": 100
}
```

## 🔧 Настройка Arduino

Прошивка Arduino должна поддерживать:
- **Скорость:** 115200 бод
- **Протокол I2C:** Адрес `0x04`
- **Формат пакета:** Структура `RpiPkg` с CRC16 проверкой

Загрузите скетч из `robo_arduino/robo_arduino.ino`

## ❓ Troubleshooting

### Не видно видеопоток
```bash
# Проверить установку webrtc-streamer
which webrtc-streamer

# Проверить камеру
ls /dev/video*

# Проверить права
sudo usermod -aG video $USER
```

### Ошибка подключения к Arduino
```bash
# Проверить устройство
ls /dev/ttyUSB* /dev/ttyACM*

# Изменить путь в src/core/main.cpp
# Добавить пользователя в группу
sudo usermod -aG dialout $USER
```

### Телефон не видит робота
- Убедитесь что телефон и Raspberry Pi в одной Wi-Fi сети
- Проверьте IP адрес: `hostname -I`
- Отключите фаервол: `sudo ufw disable`

### Конфликт портов 8080
Если WebRTC и Python Bridge конфликтуют за порт 8080:
- Измените `HTTP_PORT` в `python_bridge/bridge.py`
- Или настройте webrtc-streamer на другой порт: `-H 0.0.0.0:8000`

## 📚 Дополнительная документация

- [ARCHITECTURE.md](ARCHITECTURE.md) - Детальное описание архитектуры
- [robo_chassis/README.md](robo_chassis/README.md) - Документация C++ ядра

## 🛠 Требования к железу

- **Raspberry Pi 2 Model B** (или новее)
- **Камера Raspberry Pi** (v1/v2) или USB камера
- **Arduino Uno/Nano** с Motor Shield
- **MPU6050** (гироскоп/акселерометр)
- **Wi-Fi адаптер** (встроенный или внешний)

## 📄 Лицензия

MIT License
