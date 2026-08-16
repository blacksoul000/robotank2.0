# WebRTC Видеостриминг для RoboChassis

## Обзор

Этот модуль обеспечивает потоковую передачу видео с Raspberry Pi 2 на телефон через Wi-Fi с использованием:
- **Аппаратного кодирования H.264** (omxh264enc) для минимальной нагрузки на CPU
- **Отдельного процесса** для изоляции от основного приложения управления
- **GStreamer** для обработки видеопотока
- **UDP/RTP** для передачи с низкой задержкой

## Архитектура

```
┌─────────────────┐      Wi-Fi      ┌─────────────────┐
│  Raspberry Pi 2 │ ◄──────────────►│    Телефон      │
│                 │                 │                 │
│ ┌─────────────┐ │   UDP/H.264     │ ┌─────────────┐ │
│ │webrtc_streamer│ ├──────────────►│ │ Веб-браузер │ │
│ │  (отдельный  │ │   порт 8554    │ │ или WebView │ │
│ │   процесс)   │ │                 │ └─────────────┘ │
│ └─────────────┘ │                 │                 │
│                 │                 │ ┌─────────────┐ │
│ ┌─────────────┐ │                 │ │ Управление  │ │
│ │ robo_chassis│ │   Bluetooth/WiFi│ │ (джойстики) │ │
│ │  (управление)◄──────────────────┤               │ │
│ └─────────────┘ │                 │ └─────────────┘ │
└─────────────────┘                 └─────────────────┘
```

## Сборка

### Требования
- Raspberry Pi 2 Model B
- Raspbian Buster или новее
- GStreamer 1.0 с плагинами
- Qt5 Core и Network
- Камера Raspberry Pi или USB-камера

### Установка зависимостей на RPi

```bash
sudo apt-get update
sudo apt-get install -y \
    libgstreamer1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-omx \
    libqt5core5 \
    libqt5network5 \
    qtbase5-dev
```

### Компиляция

```bash
cd /workspace/robo_chassis
mkdir build && cd build
cmake .. -DPICAM=ON  # Включить поддержку камеры RPi
make -j4
```

Будет создан исполняемый файл `webrtc_streamer` в папке `build/webrtc_streamer/`.

## Использование

### Запуск стримера

```bash
./webrtc_streamer --host <IP_телефона> [опции]
```

#### Обязательные параметры:
- `--host <IP>` или `-H <IP>` — IP-адрес телефона в сети Wi-Fi

#### Опциональные параметры:
- `--port <PORT>` или `-p <PORT>` — Порт UDP (по умолчанию: 8554)
- `--width <WIDTH>` или `-w <WIDTH>` — Ширина кадра (по умолчанию: 640)
- `--height <HEIGHT>` или `-h <HEIGHT>` — Высота кадра (по умолчанию: 480)
- `--fps <FPS>` или `-f <FPS>` — Частота кадров (по умолчанию: 30)

#### Примеры:

```bash
# Базовый запуск
./webrtc_streamer --host 192.168.1.100

# С настройкой разрешения
./webrtc_streamer --host 192.168.1.100 --width 640 --height 480 --fps 30

# С другим портом
./webrtc_streamer --host 192.168.1.100 --port 9000
```

### На стороне телефона

#### Вариант 1: Веб-интерфейс (рекомендуется)

Создайте простой HTML-файл с WebRTC-плеером:

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>RoboChassis Video</title>
    <style>
        body { margin: 0; background: #000; }
        video { width: 100%; height: 100vh; object-fit: contain; }
        .controls { 
            position: fixed; 
            bottom: 20px; 
            left: 0; 
            right: 0; 
            display: flex; 
            justify-content: space-around;
        }
        .joystick { 
            width: 100px; 
            height: 100px; 
            background: rgba(255,255,255,0.3); 
            border-radius: 50%;
            touch-action: none;
        }
    </style>
</head>
<body>
    <video id="video" autoplay playsinline></video>
    
    <div class="controls">
        <div id="leftStick" class="joystick"></div>
        <div id="rightStick" class="joystick"></div>
    </div>

    <script>
        // Подключение к видеопотоку через WebRTC
        const video = document.getElementById('video');
        
        // Простое подключение через MSE или нативный H.264 поток
        // Для Android можно использовать ExoPlayer
        // Для iOS - AVPlayer
        
        // Пример для Android WebView:
        video.src = 'http://<RPi_IP>:8554/stream.m3u8';
        
        // Либо используйте библиотеку WebRTC
        // https://github.com/mpromonet/webrtc-streamer
    </script>
</body>
</html>
```

#### Вариант 2: Нативное приложение

**Android:**
- Используйте ExoPlayer с поддержкой H.264
- Или интегрируйте webrtc-streamer через WebView

**iOS:**
- Используйте AVPlayer для воспроизведения HLS/DASH потока
- Или WebRTC.framework для прямого WebRTC подключения

#### Вариант 3: VLC Player

Простейший способ проверки:
```
vlc udp://@:8554
```

На телефоне откройте VLC и укажите сеть → UDP поток.

## Настройка производительности

### Для Raspberry Pi 2

Оптимальные настройки для баланса качества и задержки:

```bash
# Разрешение 640x480, 30 FPS, битрейт 800 kbps
./webrtc_streamer --host 192.168.1.100 \
    --width 640 --height 480 --fps 30
```

### Параметры кодирования

В коде `webrtc_streamer.cpp` можно настроить:
- `target-bitrate` — битрейт (по умолчанию 800000 = 800 kbps)
- `control-rate` — режим контроля битрейта (`low`, `constant`, `variable`)
- `preset` — пресет энкодера (`highpower`, `ultrafast`)

## Устранение неполадок

### Видео не передается

1. Проверьте, что телефон и RPi в одной сети Wi-Fi
2. Убедитесь, что порт не заблокирован фаерволом:
   ```bash
   sudo ufw allow 8554/udp
   ```
3. Проверьте доступность RPi:
   ```bash
   ping 192.168.1.<RPi_IP>
   ```

### Высокая задержка

1. Уменьшите разрешение до 320x240
2. Снизьте FPS до 15-20
3. Уменьшите битрейт в пайплайне GStreamer

### Ошибки GStreamer

Запустите с подробным логированием:
```bash
GST_DEBUG=3 ./webrtc_streamer --host 192.168.1.100
```

### Камера не обнаружена

Для USB-камеры:
```bash
ls -la /dev/video*
v4l2-ctl --list-devices
```

Для камеры RPi:
```bash
raspistill -o test.jpg
```

## Интеграция с основным приложением

WebRTC-стример работает как отдельный процесс и не требует изменений в `robo_chassis`.

Для автоматического запуска добавьте в systemd:

```ini
# /etc/systemd/system/webrtc-streamer.service
[Unit]
Description=WebRTC Video Streamer
After=network.target

[Service]
Type=simple
ExecStart=/opt/robo_chassis/webrtc_streamer --host 192.168.1.100
Restart=on-failure
User=pi

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl enable webrtc-streamer
sudo systemctl start webrtc-streamer
```

## Альтернативы

Если WebRTC не подходит, рассмотрите:
- **MJPEG over HTTP** — проще, но выше задержка
- **RTSP** — хорошая совместимость, средняя задержка
- **NVIDIA Jetson** — аппаратное кодирование лучше для сложных кодеков

## Лицензия

MIT License
