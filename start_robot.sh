#!/bin/bash
# Скрипт запуска всей системы управления роботом

echo "🚀 Запуск системы управления роботом..."

# 1. Запуск webrtc-streamer (отдельный процесс для видео)
echo "📹 Запуск webrtc-streamer..."
/usr/local/bin/webrtc-streamer -H ./webrtc-streamer/media &
WEBRTC_PID=$!
sleep 2

# 2. Запуск основного C++ приложения robo_chassis
echo "🤖 Запуск robo_chassis..."
cd /workspace/robo_chassis/build
./robo_chassis &
ROBO_PID=$!
sleep 2

# 3. Запуск Python Bridge (TCP <-> WebSocket + HTTP)
echo "🌉 Запуск Python Bridge..."
cd /workspace/python_bridge
python3 bridge.py &
BRIDGE_PID=$!
sleep 2

echo ""
echo "✅ Все сервисы запущены!"
echo ""
echo "📱 Откройте в браузере телефона: http://<IP_RASPBERRY_PI>:8080"
echo ""
echo "Сервисы:"
echo "  - WebRTC Streamer: http://<IP>:8080/webrtc-streamer/"
echo "  - Python Bridge WebSocket: ws://<IP>:8765"
echo "  - Python Bridge HTTP: http://<IP>:8080"
echo "  - TCP Server (C++): localhost:5555"
echo ""
echo "Нажмите Ctrl+C для остановки всех сервисов"

# Ожидание прерывания
trap "kill $WEBRTC_PID $ROBO_PID $BRIDGE_PID 2>/dev/null; exit" INT TERM

wait
