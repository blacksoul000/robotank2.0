#!/bin/bash

# Скрипт установки RoboChassis на Raspberry Pi
# Запускать от имени пользователя pi (не root)
# Поддержка разных пользователей через аргумент или текущую переменную окружения

set -e

# Получение текущего пользователя
CURRENT_USER="${1:-$USER}"
if [ -z "$CURRENT_USER" ]; then
    CURRENT_USER=$(whoami)
fi

HOME_DIR="/home/$CURRENT_USER"
PROJECT_NAME="robo_chassis"
LOG_DIR="/var/log/$PROJECT_NAME"

echo "=== Установка RoboChassis ==="
echo "Пользователь: $CURRENT_USER"
echo "Директория установки: $HOME_DIR/$PROJECT_NAME"

# 0. Проверка зависимостей перед установкой
echo "[0/9] Проверка зависимостей..."
REQUIRED_PACKAGES="cmake build-essential libi2c-dev i2c-tools python3 python3-pip git libssl-dev libwslay-dev nlohmann-json3-dev"
MISSING_PACKAGES=""

for pkg in $REQUIRED_PACKAGES; do
    if ! dpkg -l | grep -q "^ii  $pkg "; then
        MISSING_PACKAGES="$MISSING_PACKAGES $pkg"
    fi
done

if [ -n "$MISSING_PACKAGES" ]; then
    echo "Отсутствуют пакеты:$MISSING_PACKAGES"
    echo "Установка недостающих зависимостей..."
    sudo apt-get update
    sudo apt-get install -y $MISSING_PACKAGES
else
    echo "Все зависимости установлены"
fi

# 1. Создание директорий
echo "[1/9] Создание директорий..."
sudo mkdir -p "$LOG_DIR"
sudo chown "$CURRENT_USER:$CURRENT_USER" "$LOG_DIR"
mkdir -p "$HOME_DIR/$PROJECT_NAME"

# 2. Установка системных зависимостей (если не были установлены на шаге 0)
echo "[2/9] Проверка дополнительных зависимостей..."
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    libi2c-dev \
    i2c-tools \
    python3 \
    python3-pip \
    git \
    libssl-dev \
    libwslay-dev \
    nlohmann-json3-dev || true

# 3. Включение I2C интерфейса
echo "[3/9] Настройка I2C..."
if ! grep -q "dtparam=i2c_arm=on" /boot/config.txt 2>/dev/null; then
    echo "dtparam=i2c_arm=on" | sudo tee -a /boot/config.txt
    echo "I2C включен. Требуется перезагрузка."
else
    echo "I2C уже включен"
fi

# 4. Копирование файлов проекта
echo "[4/9] Копирование файлов проекта..."
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp -r "$PROJECT_DIR"/* "$HOME_DIR/$PROJECT_NAME/"
cd "$HOME_DIR/$PROJECT_NAME"

# 5. Сборка проекта с rollback при ошибке
echo "[5/9] Сборка проекта..."
BACKUP_DIR="$HOME_DIR/${PROJECT_NAME}_backup_$(date +%Y%m%d_%H%M%S)"
if [ -d "build" ]; then
    echo "Сохранение предыдущей версии в $BACKUP_DIR"
    cp -r build "$BACKUP_DIR" || true
fi

mkdir -p build
cd build
if ! cmake ..; then
    echo "Ошибка cmake! Откат изменений..."
    if [ -d "$BACKUP_DIR" ]; then
        rm -rf build
        cp -r "$BACKUP_DIR" build
    fi
    exit 1
fi

if ! make -j4; then
    echo "Ошибка сборки! Откат изменений..."
    cd ..
    if [ -d "$BACKUP_DIR" ]; then
        rm -rf build
        cp -r "$BACKUP_DIR" build
        echo "Предыдущая версия восстановлена из $BACKUP_DIR"
    fi
    exit 1
fi
cd ..

# Очистка старых бэкапов (оставляем последние 3)
ls -dt "$HOME_DIR"/${PROJECT_NAME}_backup_* 2>/dev/null | tail -n +4 | xargs rm -rf 2>/dev/null || true

# 6. Настройка прав доступа к устройствам
echo "[6/9] Настройка прав доступа..."
cat << EOF | sudo tee /etc/udev/rules.d/99-robochassis.rules
# Разрешить пользователю $CURRENT_USER доступ к USB UART и I2C
KERNEL=="ttyUSB*", MODE="0666"
KERNEL=="i2c-*", MODE="0666"
EOF
sudo udevadm control --reload-rules

# 7. Установка systemd сервиса с поддержкой разных пользователей
echo "[7/9] Установка systemd сервиса..."
# Создаем инстанс сервиса для конкретного пользователя
sudo sed "s/%i/$CURRENT_USER/g" deploy/robo-chassis.service | sudo tee "/etc/systemd/system/robo-chassis@$CURRENT_USER.service" > /dev/null
sudo systemctl daemon-reload
sudo systemctl enable "robo-chassis@$CURRENT_USER"

# 8. Добавление пользователя в группы
echo "[8/9] Добавление пользователя в группы..."
sudo usermod -aG dialout,i2c "$CURRENT_USER"

# 9. Проверка установки
echo "[9/9] Проверка установки..."
if [ -f "build/robo_chassis" ]; then
    echo "✓ Бинарный файл собран успешно"
else
    echo "✗ Ошибка: бинарный файл не найден"
    exit 1
fi

echo ""
echo "=== Установка завершена ==="
echo ""
echo "Доступные команды:"
echo "  sudo systemctl start robo-chassis@$CURRENT_USER    - Запуск сервиса"
echo "  sudo systemctl stop robo-chassis@$CURRENT_USER     - Остановка сервиса"
echo "  sudo systemctl status robo-chassis@$CURRENT_USER   - Проверка статуса"
echo "  journalctl -u robo-chassis@$CURRENT_USER -f        - Просмотр логов"
echo ""
echo "Для применения изменений групп требуется перезайти в систему."
echo "Если I2C был только что включен, требуется перезагрузка:"
echo "  sudo reboot"
echo ""

# Rollback инструкция
echo "В случае проблем можно восстановить предыдущую версию:"
echo "  cd $HOME_DIR/$PROJECT_NAME"
echo "  sudo systemctl stop robo-chassis@$CURRENT_USER"
echo "  rm -rf build"
echo "  cp -r ${PROJECT_NAME}_backup_* build  # выберите нужный бэкап"
echo "  sudo systemctl start robo-chassis@$CURRENT_USER"
