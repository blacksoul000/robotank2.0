#!/bin/bash

# Скрипт запуска всех компонентов робота

echo "=== Запуск RoboChassis ==="

# Переход в директорию проекта
cd "$(dirname "$0")"

# Функция для остановки всех процессов при завершении
cleanup() {
    echo "Остановка всех сервисов..."
    kill $CPP_PID 2>/dev/null
    kill $PYTHON_PID 2>/dev/null
    kill $WEBRTC_PID 2>/dev/null
    wait
    echo "Все сервисы остановлены."
    exit 0
}

trap cleanup SIGINT SIGTERM

# 1. Запуск C++ ядра
echo "[1/3] Запуск C++ ядра (robo_chassis)..."
./build/robo_chassis &
CPP_PID=$!
sleep 1

# Проверка, запустилось ли C++ приложение
if ! kill -0 $CPP_PID 2>/dev/null; then
    echo "Ошибка: не удалось запустить C++ ядро!"
    exit 1
fi

# 2. Запуск Python моста
echo "[2/3] Запуск Python Bridge..."
python3 python_bridge/bridge.py &
PYTHON_PID=$!
sleep 2

# Проверка, запустился ли Python
if ! kill -0 $PYTHON_PID 2>/dev/null; then
    echo "Ошибка: не удалось запустить Python Bridge!"
    kill $CPP_PID
    exit 1
fi

# 3. Запуск webrtc-streamer (если установлен)
echo "[3/3] Проверка webrtc-streamer..."
if command -v webrtc-streamer &> /dev/null; then
    # Запускаем с параметрами для Raspberry Pi Camera
    webrtc-streamer -H 0.0.0.0:8000 rpi:///dev/video0 &
    WEBRTC_PID=$!
    echo "WebRTC стример запущен на порту 8000"
else
    echo "webrtc-streamer не найден. Видео будет недоступно."
    echo "Установите: sudo apt install webrtc-streamer"
    WEBRTC_PID=""
fi

echo ""
echo "=== Все сервисы запущены ==="
echo ""
echo "Откройте в браузере телефона:"
echo "  http://<IP_RASPBERRY_PI>:8080"
echo ""
echo "Видеопоток (если webrtc-streamer установлен):"
echo "  http://<IP_RASPBERRY_PI>:8000"
echo ""
echo "Нажмите Ctrl+C для остановки всех сервисов"
echo ""

# Ожидание завершения
wait
