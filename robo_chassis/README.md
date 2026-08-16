# RoboChassis - Система управления роботом-танком

Современная система управления роботом на базе Raspberry Pi 2 Model B с управлением через веб-интерфейс на телефоне.

## Архитектура

```
Телефон (браузер)
    ↓ Wi-Fi (HTTP/WebSocket)
Python Bridge (aiohttp + websockets)
    ↓ TCP (localhost:5555)
C++ ядро (robo_chassis)
    ↓ UART/I²C
Arduino → Моторы, сервы, датчики
```

**Видеопоток:** Отдельный процесс `webrtc-streamer` передаёт видео с камеры через WebRTC напрямую в браузер.

## Компоненты

### 1. C++ Ядро (`src/core/`)
- **main.cpp** - точка входа, управление потоками
- **tcp_server.cpp** - TCP сервер для приема команд от Python Bridge (порт 5555)
- **serial_port.cpp** - работа с UART через termios (без Qt)
- **robot_logic.cpp** - логика управления, обработка телеметрии

### 2. Python Bridge (`python_bridge/`)
- **bridge.py** - мост между браузером и C++:
  - HTTP сервер (порт 8080) - раздает веб-интерфейс
  - WebSocket сервер (порт 8765) - обмен данными с браузером
  - TCP клиент - соединение с C++ ядром

### 3. Веб-интерфейс (`static/`)
- **index.html** - адаптивный UI с:
  - Два сенсорных джойстика (движение + башня)
  - Кнопка "ОГОНЬ"
  - Кнопка "СВЕТ"
  - Панель телеметрии (батарея, крен, тангаж, башня)
  - Видеопоток на фоне

### 4. WebRTC Streamer
- Отдельный системный сервис для трансляции видео
- Аппаратное кодирование H.264 через VPU Raspberry Pi
- Минимальная задержка (~100-300 мс)

## Зависимости

### Системные (Debian/Raspbian)
```bash
sudo apt update
sudo apt install -y \
    cmake \
    g++ \
    make \
    libjsoncpp-dev \
    python3 \
    python3-pip \
    webrtc-streamer
```

### Python зависимости
```bash
pip3 install aiohttp websockets
```

### Опционально (для работы с камерой)
```bash
sudo apt install -y \
    libcamera-dev \
    v4l-utils
```

## Сборка

```bash
cd /path/to/robo_chassis
mkdir -p build
cd build
cmake ..
make -j4
```

## Запуск

### Автоматический запуск всех компонентов
```bash
./start_robot.sh
```

### Ручной запуск по отдельности

1. **C++ ядро:**
```bash
./build/robo_chassis
```

2. **Python Bridge:**
```bash
cd python_bridge
python3 bridge.py
```

3. **WebRTC стример:**
```bash
webrtc-streamer -H 0.0.0.0:8000 rpi:///dev/video0
```

## Использование

1. Запустите все компоненты через `./start_robot.sh`
2. Узнайте IP адрес Raspberry Pi: `hostname -I`
3. Откройте в браузере телефона: `http://<IP_RPI>:8080`
4. Разрешите доступ к полноэкранному режиму
5. Управляйте роботом с помощью джойстиков

## Форматы сообщений

### Команда управления (Browser → Python → C++)
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

### Телеметрия (C++ → Python → Browser)
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

## Преимущества новой архитектуры

✅ **Чистый C++20** - никаких зависимостей от Qt  
✅ **Минимальное потребление памяти** - ~5 МБ вместо ~150 МБ  
✅ **Разделение процессов** - падение видео не влияет на управление  
✅ **Низкая задержка** - WebRTC обеспечивает 100-300 мс  
✅ **Кроссплатформенность** - работает на любом устройстве с браузером  
✅ **Простота развертывания** - один скрипт запускает всё  

## Структура проекта

```
robo_chassis/
├── src/core/              # C++ ядро (чистый C++20)
│   ├── main.cpp
│   ├── tcp_server.hpp/cpp
│   ├── serial_port.hpp/cpp
│   └── robot_logic.hpp/cpp
├── python_bridge/         # Python мост
│   └── bridge.py
├── static/                # Веб-интерфейс
│   └── index.html
├── CMakeLists.txt
├── start_robot.sh
└── README.md
```

## Удаленные компоненты

Следующие компоненты были удалены как избыточные:
- ❌ MAVLINK и вся его обвязка
- ❌ Bluetooth Manager
- ❌ OpenCV и tracking
- ❌ RTSP сервер
- ❌ robo_gui (Qt приложение)
- ❌ Gamepad Controller (теперь геймпад подключается к телефону)
- ❌ Все зависимости Qt5

## Настройка Arduino

Прошивка Arduino должна поддерживать следующий протокол:
- Скорость: 115200 бод
- Формат пакета моторов: `'M' + 5 байт данных + '\n'`
- Байты данных: left_y, left_x, right_y, right_x, flags (0-255)

## Troubleshooting

### Не видно видеопоток
- Проверьте установку webrtc-streamer: `which webrtc-streamer`
- Убедитесь что камера подключена: `ls /dev/video*`
- Проверьте права доступа: `sudo usermod -aG video $USER`

### Ошибка подключения к Arduino
- Проверьте путь к устройству: `ls /dev/ttyUSB*` или `ls /dev/ttyACM*`
- Измените путь в `src/core/main.cpp`
- Добавьте пользователя в группу dialout: `sudo usermod -aG dialout $USER`

### Телефон не видит робота
- Убедитесь что телефон и Raspberry Pi в одной Wi-Fi сети
- Проверьте IP адрес: `hostname -I`
- Отключите фаервол: `sudo ufw disable`

## Лицензия

MIT License
