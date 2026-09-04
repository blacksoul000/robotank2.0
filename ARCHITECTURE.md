# Архитектура управления роботом (Wi-Fi)

[![Version](https://img.shields.io/badge/version-1.0-blue.svg)](../README.md)
[![Last Updated](https://img.shields.io/badge/updated-2024-green.svg)]()

## Общая схема

```
┌─────────────────┐
│ Телефон         │
│ (браузер)       │
│ - Web UI        │
│ - Джойстики     │
│ - Телеметрия    │
└────────┬────────┘
         │ Wi-Fi
         │ HTTP/WebSocket
         ↓
┌─────────────────┐
│ Python Bridge   │
│ (aiohttp +      │
│  websockets)    │
│ - HTTP: 8080    │
│ - WS: 8765      │
│ - TCP клиент    │
└────────┬────────┘
         │ TCP
         │ localhost:5555
         ↓
┌─────────────────┐
│ C++ robo_chassis│
│ - TCP сервер    │
│ - RobotLogic    │
│ - UART/I2C      │
│ - MPU6050       │
└────────┬────────┘
         │ I²C/UART
         ↓
┌─────────────────┐
│ Arduino         │
│ - Motor Control │
│ - Current Sense │
│ - Voltage Mon.  │
└────────┬────────┘
         │
         ↓
  Моторы, сервы,
  датчики, фары
```

**Видеопоток (отдельный процесс):**
```
Камера → rpicam-vid → RTSP → MediaMTX → HLS/WebRTC → Браузер
(порт 8554/8889)
```

## Компоненты

### 1. MediaMTX + rpicam-vid (отдельный процесс)

- **Путь:** системный пакет `mediamtx` + `rpicam-vid`
- **Назначение:** Трансляция видео с камеры через RTSP сервер
- **Порты:** 
  - 8554 (RTSP для приема потока)
  - 8889 (HLS для веб-интерфейса)
  - 8890 (WebRTC для низкой задержки)
- **Аппаратное кодирование:** H.264 через VPU Raspberry Pi 2
- **Запуск:** 
  ```bash
  mediamtx mediamtx.yml &
  rpicam-vid -t 0 --codec libav --libav-format h264 \
    --libav-video-codec h264_v4l2m2m --width 640 --height 480 \
    --framerate 30 --bitrate 1000000 --intra 15 --inline -o - | \
    ffmpeg -f h264 -i /dev/stdin -c copy -f rtsp \
    rtsp://127.0.0.1:8554/stream
  ```

### 2. Python Bridge

- **Путь:** `python_bridge/bridge.py` или `robo_chassis/python_bridge/bridge.py`
- **Назначение:** Мост между C++ и браузером
- **Функции:**
  - HTTP сервер (порт 8080) - раздает веб-интерфейс
  - WebSocket сервер (порт 8765) - обмен данными с браузером
  - TCP клиент (порт 5555) - соединение с C++ приложением
- **Зависимости:** `aiohttp>=3.8.0`, `websockets>=10.0`
- **Установка:** `pip3 install aiohttp websockets`

### 3. C++ robo_chassis

- **Путь:** `robo_chassis/src/core/`
- **Язык:** C++20 (чистый, без Qt)
- **Основные компоненты:**

| Файл | Назначение |
|------|------------|
| `main.cpp` | Точка входа, управление потоками |
| `tcp_server.hpp/cpp` | TCP сервер на порту 5555 |
| `serial_port.hpp/cpp` | UART через termios (без Qt) |
| `robot_logic.hpp/cpp` | Логика управления, обработка команд |
| `exchangers/uart_exchanger.*` | Обмен с Arduino по UART |
| `exchangers/i2c_master.*` | I²C мастер для MPU6050 |
| `gpio/gpio_controller.*` | Управление GPIO |
| `gpio/mpu6050_imu.*` | Драйвер гироскопа/акселерометра |
| `gpio/complementary_filter.*` | Фильтр для ориентации |

- **Потребление памяти:** ~5 МБ
- **Частота обновления:** 50 Гц (20 мс)

### 4. Веб-интерфейс

- **Путь:** `robo_chassis/static/index.html`
- **Технологии:** HTML5, CSS3, Vanilla JavaScript
- **Компоненты:**
  - Два сенсорных джойстика (левый - движение, правый - башня)
  - Кнопки "ОГОНЬ" и "СВЕТ"
  - Панель телеметрии в реальном времени
  - Видеопоток на фоне (iframe с HLS/WebRTC от MediaMTX)
- **Адаптивность:** Полная поддержка мобильных устройств

### 5. Arduino Firmware

- **Путь:** `robo_arduino/robo_arduino.ino`
- **Платформа:** Arduino Uno/Nano
- **Протокол связи:** I2C (ведомый, адрес 0x04)
- **Функции:**
  - Управление 3 моторами (левый, правый, башня)
  - Мониторинг тока (3 датчика)
  - Контроль напряжения батареи
  - Защита от низкого напряжения
  - Энергосбережение (LowPower library)

## Поток данных

### Управление (Телефон → Робот)

```
1. Пользователь двигает джойстики в браузере
         ↓
2. JavaScript отправляет JSON через WebSocket (порт 8765)
         ↓
3. Python Bridge получает сообщение и пересылает в TCP сокет (localhost:5555)
         ↓
4. C++ TcpServer принимает JSON, парсит команду
         ↓
5. RobotLogic вычисляет управляющие воздействия для моторов
         ↓
6. UartExchanger формирует пакет и отправляет на Arduino по I2C
         ↓
7. Arduino применяет PWM к моторам и сервам
```

**Время отклика:** ~50-100 мс (включая сеть Wi-Fi)

### Телеметрия (Робот → Телефон)

```
1. GpioController читает данные с MPU6050 (крен, тангаж)
         ↓
2. Arduino отправляет телеметрию (напряжение, ток) через I2C
         ↓
3. RobotLogic агрегирует данные, вычисляет угол башни
         ↓
4. WifiTelemetrySender формирует JSON и отправляет в TCP сокет
         ↓
5. Python Bridge читает строку, парсит JSON
         ↓
6. Отправляет данные всем подключенным WebSocket клиентам
         ↓
7. JavaScript обновляет отображение телеметрии (60 FPS)
```

**Частота обновления:** 50 Гц (C++ → Python), 60 Гц (Python → Browser)

### Видео (Камера → Телефон)

```
1. Камера захватывает видео (/dev/video0)
         ↓
2. rpicam-vid кодирует H.264 через VPU Raspberry Pi
         ↓
3. Поток передается через RTSP в MediaMTX
         ↓
4. MediaMTX транслирует через HLS/WebRTC в браузер
         ↓
5. Воспроизведение через HTML5 video element
```

**Задержка:** 100-300 мс  
**Разрешение:** до 1920x1080 @ 30fps  
**Битрейт:** 2-6 Мбит/с (адаптивный)

## Форматы сообщений

### Команда управления (Телефон → C++)

```json
{
  "type": "COMMAND",
  "left_x": 0.0,        // Левый джойстик X (-1.0 .. 1.0)
  "left_y": -0.5,       // Левый джойстик Y (-1.0 .. 1.0)
  "right_x": 0.3,       // Правый джойстик X (-1.0 .. 1.0)
  "right_y": 0.0,       // Правый джойстик Y (-1.0 .. 1.0)
  "fire": false,        // Огонь (true/false)
  "lights": true        // Фары (true/false)
}
```

**Поля:**
- `left_x/y` - управление движением (газ/поворот)
- `right_x/y` - управление башней (горизонтально/вертикально)
- `fire` - активация сервопривода стрельбы
- `lights` - включение/выключение фар

### Телеметрия (C++ → Телефон)

```json
{
  "type": "TELEMETRY",
  "battery": 12.4,          // Напряжение батареи (V)
  "roll": 2.5,              // Крен (градусы)
  "pitch": -1.3,            // Тангаж (градусы)
  "yaw": 45.0,              // Рыскание (градусы)
  "turret_angle": 45.0,     // Угол башни (градусы)
  "current_left": 150,      // Ток левого мотора (mA)
  "current_right": 145,     // Ток правого мотора (mA)
  "current_tower": 80,      // Ток мотора башни (mA)
  "signal_quality": 100,    // Качество сигнала (%)
  "arduino_online": true,   // Статус Arduino
  "gyro_ready": true        // Статус гироскопа
}
```

**Частота обновления:** 50 Гц  
**Формат:** JSON строка с разделителем `\n`

## Запуск

### Автоматический запуск всех компонентов

```bash
# Установка зависимостей Python
pip3 install aiohttp websockets

# Сборка C++ ядра
cd robo_chassis
mkdir -p build && cd build
cmake .. && make -j4

# Запуск всех сервисов
./start_robot.sh
```

### Ручной запуск по отдельности

```bash
# 1. C++ ядро (в одном терминале)
./build/robo_chassis

# 2. Python Bridge (в другом терминале)
python3 python_bridge/bridge.py

# 3. MediaMTX + rpicam-vid (в третьем терминале)
mediamtx mediamtx.yml &
rpicam-vid -t 0 --codec libav --libav-format h264 \
  --libav-video-codec h264_v4l2m2m --width 640 --height 480 \
  --framerate 30 --bitrate 1000000 --intra 15 --inline -o - | \
  ffmpeg -f h264 -i /dev/stdin -c copy -f rtsp \
  rtsp://127.0.0.1:8554/stream
```

## Порты

| Сервис | Порт | Протокол | Описание |
|--------|------|----------|----------|
| Python Bridge HTTP | 8080 | HTTP | Веб-интерфейс |
| Python Bridge WebSocket | 8765 | WebSocket | Обмен данными с браузером |
| C++ TCP Server | 5555 | TCP | Связь Python ↔ C++ |
| MediaMTX RTSP | 8554 | RTSP | Прием видеопотока |
| MediaMTX HLS | 8889 | HTTP | HLS трансляция |
| MediaMTX WebRTC | 8890 | HTTPS | WebRTC трансляция |

**Примечание:** MediaMTX использует порты 8554, 8889, 8890 для видеотрансляции. Убедитесь, что эти порты не заняты другими сервисами.

## Преимущества архитектуры

1. **Изоляция процессов** - падение одного сервиса не влияет на другие
2. **Минимальная задержка** - rpicam-vid + MediaMTX обеспечивают ~100-300 мс
3. **Кроссплатформенность** - управление с любого устройства с браузером
4. **Простота расширения** - легко добавить новые датчики или команды
5. **Чистый C++20** - без тяжелых зависимостей типа Qt
6. **Низкое потребление памяти** - ~5 МБ для C++ ядра
7. **Аппаратное ускорение** - кодирование H.264 через VPU Raspberry Pi

## Безопасность

⚠️ **Внимание:** Текущая архитектура не включает механизмы безопасности:

- Нет аутентификации пользователей
- Нет шифрования трафика (HTTP/WebSocket вместо HTTPS/WSS)
- Нет защиты от DoS-атак
- Открытые порты доступны в локальной сети

**Рекомендации для продакшена:**
- Добавить базовую аутентификацию
- Использовать reverse proxy (nginx) с SSL
- Ограничить доступ по IP/MAC адресу
- Реализовать rate limiting

## Диагностика

### Проверка статусов

```bash
# Проверка работы C++ ядра
ps aux | grep robo_chassis

# Проверка Python Bridge
ps aux | grep bridge.py

# Проверка MediaMTX
ps aux | grep mediamtx

# Проверка rpicam-vid/ffmpeg
ps aux | grep -E 'rpicam-vid|ffmpeg'

# Проверка портов
netstat -tlnp | grep -E '8080|8765|5555|8554|8889|8890'

# Проверка камеры
ls -la /dev/video*

# Проверка I2C устройств
i2cdetect -y 1

# Проверка UART
ls -la /dev/ttyUSB* /dev/ttyACM*
```

### Логирование

```bash
# Запуск с перенаправлением логов
./build/robo_chassis 2>&1 | tee chassis.log
python3 python_bridge/bridge.py 2>&1 | tee bridge.log
```
