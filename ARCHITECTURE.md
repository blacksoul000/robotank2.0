# Архитектура управления роботом (Wi-Fi)

## Общая схема

```
Телефон (браузер)
    ↓ Wi-Fi (HTTP/WebSocket)
Python Bridge (aiohttp)
    ↓ TCP (localhost:5555)
C++ robo_chassis (TCP сервер + PubSub)
    ↓ I²C/UART
Arduino (Monster Motor Shield)
    ↓
Моторы, сервы, фары
```

## Компоненты

### 1. WebRTC Streamer (отдельный процесс)
- **Путь:** `robo_chassis/src/webrtc_streamer/`
- **Назначение:** Трансляция видео с камеры по WebRTC
- **Порт:** 8080 (HTTP), 8554 (RTSP для внутреннего использования)
- **Аппаратное кодирование:** H.264 через VPU Raspberry Pi 2

### 2. Python Bridge
- **Путь:** `python_bridge/bridge.py`
- **Назначение:** Мост между C++ и браузером
- **Функции:**
  - HTTP сервер (порт 8080) - раздает веб-интерфейс
  - WebSocket сервер (порт 8765) - обмен данными с браузером
  - TCP клиент (порт 5555) - соединение с C++ приложением
- **Зависимости:** `aiohttp>=3.8.0`, `websockets>=10.0`

### 3. C++ robo_chassis
- **Путь:** `robo_chassis/`
- **Основные задачи:**
  - `TcpCommandServer` - TCP сервер на порту 5555, прием команд от Python Bridge
  - `WifiTelemetrySender` - отправка телеметрии в Python Bridge
  - `RoboCore` - основная логика управления, обработка команд
  - `ArduinoExchanger` - обмен данными с Arduino
  - `GpioController` - управление GPIO, чтение MPU6050
  - `ConfigHandler` - конфигурация

### 4. Веб-интерфейс (встроен в Python Bridge)
- **Два виртуальных джойстика** для управления
- **Отображение телеметрии:**
  - 🔋 Заряд батареи (%)
  - 📐 Крен (roll, °)
  - 📐 Тангаж (pitch, °)
  - 🎯 Угол башни (turret angle, °)
- **Видеопоток** через iframe (WebRTC)

## Поток данных

### Управление (Телефон → Робот)
1. Пользователь двигает джойстики в браузере
2. JavaScript отправляет JSON через WebSocket
3. Python Bridge получает сообщение и пересылает в TCP сокет
4. C++ TcpCommandServer принимает JSON
5. Через PubSub команда попадает в RoboCore
6. RoboCore вычисляет управляющие воздействия
7. ArduinoExchanger отправляет данные на Arduino
8. Arduino управляет моторами и сервами

### Телеметрия (Робот → Телефон)
1. GpioController читает данные с MPU6050 (крен, тангаж)
2. RoboCore знает угол башни из кинематики
3. WifiTelemetrySender формирует JSON и отправляет в TCP сокет
4. Python Bridge читает строку, парсит JSON
5. Отправляет данные всем подключенным WebSocket клиентам
6. JavaScript обновляет отображение телеметрии

### Видео (Камера → Телефон)
1. Камера подключена к Raspberry Pi
2. webrtc-streamer захватывает видео через GStreamer
3. Аппаратное кодирование H.264 через VPU
4. Передача по WebRTC в браузер
5. Воспроизведение через HTML5 video

## Форматы сообщений

### Команда (Телефон → C++)
```json
{
  "type": "COMMAND",
  "left": {"x": 0.5, "y": -0.3},
  "right": {"x": 0.0, "y": 0.8}
}
```

### Телеметрия (C++ → Телефон)
```json
{
  "type": "TELEMETRY",
  "battery": 85,
  "roll": 2.5,
  "pitch": -1.3,
  "turret_angle": 45.0
}
```

## Запуск

```bash
# Установка зависимостей Python
pip3 install aiohttp websockets

# Запуск всех сервисов
./start_robot.sh
```

## Порты

| Сервис | Порт | Протокол |
|--------|------|----------|
| WebRTC Streamer HTTP | 8080 | HTTP |
| WebRTC Streamer RTSP | 8554 | RTSP |
| Python Bridge HTTP | 8080* | HTTP |
| Python Bridge WebSocket | 8765 | WebSocket |
| C++ TCP Server | 5555 | TCP |

*Примечание: WebRTC и Python Bridge могут конфликтовать за порт 8080. 
При необходимости измените HTTP_PORT в bridge.py или настройте webrtc-streamer на другой порт.

## Преимущества архитектуры

1. **Изоляция процессов** - падение одного сервиса не влияет на другие
2. **Минимальная задержка** - WebRTC обеспечивает ~100-300 мс
3. **Кроссплатформенность** - управление с любого устройства с браузером
4. **Простота расширения** - легко добавить новые датчики или команды
5. **Отсутствие MAVLINK** - упрощенная архитектура без излишней сложности
