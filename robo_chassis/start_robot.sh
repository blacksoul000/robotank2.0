#!/bin/bash

# Скрипт запуска всех компонентов робота
# Версия: 2.0 (с health check и оптимизацией для Raspberry Pi 2B)

set -e  # Выход при ошибке

echo "=== Запуск RoboChassis ==="
echo "Дата: $(date)"
echo ""

# Переход в директорию проекта
cd "$(dirname "$0")"

# Конфигурация
HEALTH_CHECK_INTERVAL=3
HEALTH_CHECK_MAX_FAILURES=3
CAMERA_HEALTH_CHECK=true  # Включить health check для камеры
LOG_DIR="/var/log/robo_chassis"
PYTHON_BRIDGE_PORT=8080
CPP_CORE_PORT=5555
WEBSOCKET_PORT=8765
MEDIA_HTTP_PORT=8889
MEDIA_RTSP_PORT=8554
TELEMETRY_RATE_HZ=2  # Частота обновления телеметрии (Гц)

# Функция проверки существования порта (использует lsof и ss)
check_port() {
    local port=$1
    local max_attempts=${2:-10}
    local attempt=0
    
    while [ $attempt -lt $max_attempts ]; do
        # Используем lsof или ss для проверки порта (более надежно чем netstat)
        if command -v lsof &> /dev/null; then
            if lsof -i :"$port" -t &> /dev/null; then
                return 0
            fi
        elif ss -tlnp 2>/dev/null | grep -q ":$port "; then
            return 0
        elif netstat -tuln 2>/dev/null | grep -q ":$port "; then
            return 0
        fi
        attempt=$((attempt + 1))
        sleep 0.5
    done
    return 1
}

# Функция проверки что mediamtx слушает порты
check_mediamtx_ports() {
    local http_port=8889
    local rtsp_port=8554
    local max_attempts=10
    local attempt=0
    
    while [ $attempt -lt $max_attempts ]; do
        local ports_ok=true
        
        if command -v lsof &> /dev/null; then
            if ! lsof -i :"$http_port" -t &> /dev/null; then
                ports_ok=false
            fi
            if ! lsof -i :"$rtsp_port" -t &> /dev/null; then
                ports_ok=false
            fi
        elif ss -tlnp 2>/dev/null | grep -q ":$http_port " && \
             ss -tlnp 2>/dev/null | grep -q ":$rtsp_port "; then
            ports_ok=true
        else
            ports_ok=false
        fi
        
        if [ "$ports_ok" = true ]; then
            return 0
        fi
        
        attempt=$((attempt + 1))
        sleep 0.5
    done
    return 1
}

# Функция health check для процессов
health_check() {
    local pid=$1
    local name=$2
    
    if [ -z "$pid" ] || ! kill -0 "$pid" 2>/dev/null; then
        return 1
    fi
    return 0
}

# Флаг для предотвращения повторного вызова cleanup
CLEANUP_DONE=false

# Функция для остановки всех процессов при завершении
cleanup() {
    # Защита от повторного вызова
    if [ "$CLEANUP_DONE" = true ]; then
        return
    fi
    CLEANUP_DONE=true
    
    echo ""
    echo "=== Остановка всех сервисов... ==="
    
    # Отправляем SIGTERM всем процессам для graceful shutdown
    if [ -n "$CAM_PID" ] && kill -0 "$CAM_PID" 2>/dev/null; then
        echo "  → Остановка Camera стримера..."
        kill -TERM "$CAM_PID" 2>/dev/null
    fi
    
    if [ -n "$MEDIAMTX_PID" ] && kill -0 "$MEDIAMTX_PID" 2>/dev/null; then
        echo "  → Остановка MediaMTX..."
        kill -TERM "$MEDIAMTX_PID" 2>/dev/null
    fi
    
    if [ -n "$PYTHON_PID" ] && kill -0 "$PYTHON_PID" 2>/dev/null; then
        echo "  → Остановка Python Bridge..."
        kill -TERM "$PYTHON_PID" 2>/dev/null
    fi
    
    if [ -n "$CPP_PID" ] && kill -0 "$CPP_PID" 2>/dev/null; then
        echo "  → Остановка C++ ядра..."
        kill -TERM "$CPP_PID" 2>/dev/null
    fi
    
    # Ждем завершения процессов (до 10 секунд)
    echo "  → Ожидание завершения процессов..."
    WAIT_TIME=0
    MAX_WAIT=10
    while [ $WAIT_TIME -lt $MAX_WAIT ]; do
        ALL_STOPPED=true
        
        if [ -n "$CAM_PID" ] && kill -0 "$CAM_PID" 2>/dev/null; then
            ALL_STOPPED=false
        fi
        
        if [ -n "$MEDIAMTX_PID" ] && kill -0 "$MEDIAMTX_PID" 2>/dev/null; then
            ALL_STOPPED=false
        fi
        
        if [ -n "$PYTHON_PID" ] && kill -0 "$PYTHON_PID" 2>/dev/null; then
            ALL_STOPPED=false
        fi
        
        if [ -n "$CPP_PID" ] && kill -0 "$CPP_PID" 2>/dev/null; then
            ALL_STOPPED=false
        fi
        
        if [ "$ALL_STOPPED" = true ]; then
            break
        fi
        
        sleep 1
        WAIT_TIME=$((WAIT_TIME + 1))
    done
    
    # Если процессы еще висят, убиваем их принудительно
    if [ -n "$CAM_PID" ] && kill -0 "$CAM_PID" 2>/dev/null; then
        echo "  → Принудительная остановка Camera стримера..."
        kill -9 "$CAM_PID" 2>/dev/null
    fi
    
    if [ -n "$MEDIAMTX_PID" ] && kill -0 "$MEDIAMTX_PID" 2>/dev/null; then
        echo "  → Принудительная остановка MediaMTX..."
        kill -9 "$MEDIAMTX_PID" 2>/dev/null
    fi
    
    if [ -n "$PYTHON_PID" ] && kill -0 "$PYTHON_PID" 2>/dev/null; then
        echo "  → Принудительная остановка Python Bridge..."
        kill -9 "$PYTHON_PID" 2>/dev/null
    fi
    
    if [ -n "$CPP_PID" ] && kill -0 "$CPP_PID" 2>/dev/null; then
        echo "  → Принудительная остановка C++ ядра..."
        kill -9 "$CPP_PID" 2>/dev/null
    fi
    
    # Финальное ожидание завершения дочерних процессов
    wait 2>/dev/null
    
    echo "Все сервисы остановлены."
}

trap cleanup SIGINT SIGTERM EXIT

# Проверка прав доступа
if [ "$EUID" -ne 0 ]; then 
    echo "⚠️  Предупреждение: Скрипт запущен не от root. Возможны проблемы с записью в $LOG_DIR"
    echo "   Рекомендуется: sudo ./start_robot.sh"
    echo ""
fi

# Создание директории для логов
if [ ! -d "$LOG_DIR" ]; then
    echo "Создание директории для логов: $LOG_DIR"
    mkdir -p "$LOG_DIR" 2>/dev/null || {
        echo "⚠️  Не удалось создать $LOG_DIR. Логи будут писаться только в консоль."
    }
fi

# Проверка наличия собранного бинарника
if [ ! -f "./build/robo_chassis" ]; then
    echo "❌ Ошибка: Бинарный файл ./build/robo_chassis не найден!"
    echo "   Выполните сборку: cd build && cmake .. && make"
    exit 1
fi

# Проверка наличия Python Bridge (используем основной из корня проекта)
PYTHON_BRIDGE_PATH="python_bridge/bridge.py"
if [ ! -f "$PYTHON_BRIDGE_PATH" ]; then
    PYTHON_BRIDGE_PATH="./python_bridge/bridge.py"
fi

if [ ! -f "$PYTHON_BRIDGE_PATH" ]; then
    echo "❌ Ошибка: Python Bridge bridge.py не найден!"
    exit 1
fi
echo "   Python Bridge: $PYTHON_BRIDGE_PATH"

echo "=== Проверка зависимостей ==="

# 1. Запуск C++ ядра
echo "[1/4] Запуск C++ ядра (robo_chassis)..."
./build/robo_chassis > "$LOG_DIR/cpp_core.log" 2>&1 &
CPP_PID=$!
echo "      PID: $CPP_PID"

# Ожидание запуска C++ ядра
sleep 2

if ! health_check $CPP_PID "C++ Core"; then
    echo "❌ Ошибка: не удалось запустить C++ ядро!"
    echo "   Лог ошибки:"
    tail -20 "$LOG_DIR/cpp_core.log" 2>/dev/null
    exit 1
fi

if ! check_port $CPP_CORE_PORT 20; then
    echo "❌ Ошибка: C++ ядро не слушает порт $CPP_CORE_PORT"
    exit 1
fi
echo "      ✓ C++ ядро запущено (порт $CPP_CORE_PORT)"

# 2. Запуск Python моста
echo "[2/4] Запуск Python Bridge..."
python3 "$PYTHON_BRIDGE_PATH" > "$LOG_DIR/python_bridge.log" 2>&1 &
PYTHON_PID=$!
echo "      PID: $PYTHON_PID"

# Ожидание запуска Python Bridge
sleep 3

if ! health_check $PYTHON_PID "Python Bridge"; then
    echo "❌ Ошибка: не удалось запустить Python Bridge!"
    echo "   Лог ошибки:"
    tail -20 "$LOG_DIR/python_bridge.log" 2>/dev/null
    kill $CPP_PID 2>/dev/null
    exit 1
fi

if ! check_port $PYTHON_BRIDGE_PORT 20; then
    echo "❌ Ошибка: Python Bridge не слушает порт $PYTHON_BRIDGE_PORT"
    kill $CPP_PID 2>/dev/null
    exit 1
fi
echo "      ✓ Python Bridge запущен (порт $PYTHON_BRIDGE_PORT)"

# 3. Запуск MediaMTX
echo "[3/4] Запуск MediaMTX..."
CONFIG_FILE="./mediamtx.yml"
if [ -f "$CONFIG_FILE" ]; then
    mediamtx "$CONFIG_FILE" > "$LOG_DIR/mediamtx.log" 2>&1 &
else
    echo "⚠  Config not found, starting with defaults..."
    mediamtx > "$LOG_DIR/mediamtx.log" 2>&1 &
fi
MEDIAMTX_PID=$!

# Проверка что MediaMTX слушает порты (8554 RTSP и 8889 HTTP)
sleep 2
if ! check_mediamtx_ports; then
    echo "❌ Ошибка: MediaMTX не слушает порты $MEDIA_RTSP_PORT/$MEDIA_HTTP_PORT!"
    tail -20 "$LOG_DIR/mediamtx.log" 2>/dev/null
    kill $CPP_PID $PYTHON_PID 2>/dev/null
    exit 1
fi
echo "      ✓ MediaMTX запущен (порты $MEDIA_RTSP_PORT, $MEDIA_HTTP_PORT)"

# Запуск камеры с оптимизированными параметрами для RPi 2B
# Логирование вывода rpicam-vid в отдельный файл
CAMERA_LOG="$LOG_DIR/camera.log"
echo "      Запуск видеопотока (rpicam-vid → ffmpeg → MediaMTX)..."

# Унифицированный pipeline для запуска и перезапуска
CAMERA_PIPELINE="rpicam-vid -t 0 --camera 0 --nopreview --codec libav --libav-format h264 --libav-video-codec h264_v4l2m2m --width 1024 --height 768 --framerate 30 --bitrate 1000000 --intra 15 --inline -o - 2>>\"$CAMERA_LOG\" | ffmpeg -y -f h264 -i /dev/stdin -c copy -f rtsp -rtsp_transport tcp rtsp://127.0.0.1:$MEDIA_RTSP_PORT/stream 2>>\"$CAMERA_LOG\""

# Логируем параметры запуска камеры
echo "$(date): Starting camera pipeline" >> "$CAMERA_LOG"
echo "Pipeline: $CAMERA_PIPELINE" >> "$CAMERA_LOG"

eval "$CAMERA_PIPELINE" &
CAM_PID=$!

# Проверка запуска камеры
sleep 2
if ! health_check $CAM_PID "Видеопоток"; then
    echo "❌ Ошибка: не удалось запустить видеопоток!"
    echo "   Проверьте логи: $CAMERA_LOG"
    kill $CPP_PID $PYTHON_PID $MEDIAMTX_PID 2>/dev/null
    exit 1
fi
echo "      ✓ Видеопоток запущен (PID: $CAM_PID)"

# 4. Health Check мониторинг
echo "[4/4] Запуск мониторинга health check..."
echo ""
echo "=== Все сервисы запущены ==="
echo ""
echo "Интерфейс управления:"
echo "  http://<IP_RASPBERRY_PI>:$PYTHON_BRIDGE_PORT"
echo ""
echo "PID процессов:"
echo "  C++ Core:       $CPP_PID"
echo "  Python Bridge:  $PYTHON_PID"
echo "  MediaMTX:       $MEDIAMTX_PID"
[ -n "$CAM_PID" ] && echo "  Camera stream:  $CAM_PID"
echo ""
echo "Логи:"
echo "  $LOG_DIR/cpp_core.log"
echo "  $LOG_DIR/python_bridge.log"
echo "  $LOG_DIR/mediamtx.log"
echo ""
echo "Health Check: каждые ${HEALTH_CHECK_INTERVAL}c (макс. ошибок: $HEALTH_CHECK_MAX_FAILURES)"
[ "$CAMERA_HEALTH_CHECK" = "true" ] && echo "Camera Health Check: включен"
echo "Нажмите Ctrl+C для остановки всех сервисов"
echo ""

# Главный цикл с health check
cpp_failures=0
python_failures=0
camera_failures=0
mediamtx_failures=0

while true; do
    sleep $HEALTH_CHECK_INTERVAL
    
    # Проверка C++ ядра
    if ! health_check $CPP_PID "C++ Core"; then
        cpp_failures=$((cpp_failures + 1))
        echo "⚠️  C++ ядро недоступно (ошибок: $cpp_failures/$HEALTH_CHECK_MAX_FAILURES)"
        
        if [ $cpp_failures -ge $HEALTH_CHECK_MAX_FAILURES ]; then
            echo "❌ Критическая ошибка: C++ ядро упало. Перезапуск..."
            ./build/robo_chassis > "$LOG_DIR/cpp_core.log" 2>&1 &
            CPP_PID=$!
            cpp_failures=0
            sleep 2
            
            if ! health_check $CPP_PID "C++ Core"; then
                echo "❌ Не удалось перезапустить C++ ядро. Остановка."
                break
            fi
            echo "✓ C++ ядро перезапущено (PID: $CPP_PID)"
        fi
    else
        cpp_failures=0
    fi
    
    # Проверка Python Bridge
    if ! health_check $PYTHON_PID "Python Bridge"; then
        python_failures=$((python_failures + 1))
        echo "⚠️  Python Bridge недоступен (ошибок: $python_failures/$HEALTH_CHECK_MAX_FAILURES)"
        
        if [ $python_failures -ge $HEALTH_CHECK_MAX_FAILURES ]; then
            echo "❌ Критическая ошибка: Python Bridge упал. Перезапуск..."
            python3 "$PYTHON_BRIDGE_PATH" > "$LOG_DIR/python_bridge.log" 2>&1 &
            PYTHON_PID=$!
            python_failures=0
            sleep 2
            
            if ! health_check $PYTHON_PID "Python Bridge"; then
                echo "❌ Не удалось перезапустить Python Bridge. Остановка."
                break
            fi
            echo "✓ Python Bridge перезапущен (PID: $PYTHON_PID)"
        fi
    else
        python_failures=0
    fi
    
    # Проверка MediaMTX
    if ! health_check $MEDIAMTX_PID "MediaMTX"; then
        mediamtx_failures=$((mediamtx_failures + 1))
        echo "⚠️  MediaMTX недоступен (ошибок: $mediamtx_failures/$HEALTH_CHECK_MAX_FAILURES)"
        
        if [ $mediamtx_failures -ge $HEALTH_CHECK_MAX_FAILURES ]; then
            echo "❌ Критическая ошибка: MediaMTX упал. Перезапуск..."
            if [ -f "$CONFIG_FILE" ]; then
                mediamtx "$CONFIG_FILE" > "$LOG_DIR/mediamtx.log" 2>&1 &
            else
                mediamtx > "$LOG_DIR/mediamtx.log" 2>&1 &
            fi
            MEDIAMTX_PID=$!
            mediamtx_failures=0
            sleep 2
            
            if ! health_check $MEDIAMTX_PID "MediaMTX"; then
                echo "❌ Не удалось перезапустить MediaMTX. Остановка."
                break
            fi
            echo "✓ MediaMTX перезапущен (PID: $MEDIAMTX_PID)"
        fi
    else
        mediamtx_failures=0
    fi
    
    # Проверка видеопотока с авто-перезапуском (если включено)
    if [ "$CAMERA_HEALTH_CHECK" = "true" ]; then
        if [ -n "$CAM_PID" ] && ! health_check $CAM_PID "Видеопоток"; then
            camera_failures=$((camera_failures + 1))
            echo "⚠️  Видеопоток упал (ошибок: $camera_failures/$HEALTH_CHECK_MAX_FAILURES)"
            
            if [ $camera_failures -ge $HEALTH_CHECK_MAX_FAILURES ]; then
                echo "🔄 Перезапуск видеопотока..."
                
                # Унифицированный pipeline (тот же что и при старте)
                eval "$CAMERA_PIPELINE" &
                CAM_PID=$!
                camera_failures=0
                sleep 2
                
                # Проверка порта камеры после рестарта
                if ! check_port $MEDIA_RTSP_PORT 5; then
                    echo "❌ Не удалось перезапустить видеопоток (порт $MEDIA_RTSP_PORT не открыт)."
                    CAM_PID=""
                else
                    echo "✓ Видеопоток перезапущен (PID: $CAM_PID, порт: $MEDIA_RTSP_PORT)"
                    echo "$(date): Camera restarted successfully, PID=$CAM_PID" >> "$CAMERA_LOG"
                fi
            fi
        else
            camera_failures=0
        fi
    else
        # Старое поведение без авто-перезапуска
        if [ -n "$CAM_PID" ] && ! health_check $CAM_PID "Видеопоток"; then
            echo "⚠️  Видеопоток упал. Требуется ручной перезапуск."
            CAM_PID=""
        fi
    fi
done

# Выход (cleanup будет вызван автоматически через trap EXIT)
exit 0
