#!/bin/bash

# Скрипт установки RoboChassis на Raspberry Pi
# Запускать от имени пользователя pi (не root)

set -e

echo "=== Установка RoboChassis ==="

# Проверка архитектуры
if [[ "$(uname -m)" != "armv7l" && "$(uname -m)" != "aarch64" ]]; then
    echo "Предупреждение: архитектура $(uname -m) может отличаться от ожидаемой ARM"
fi

# 1. Создание директорий
echo "[1/8] Создание директорий..."
sudo mkdir -p /var/log/robo_chassis
sudo chown $USER:$USER /var/log/robo_chassis
mkdir -p ~/robo_chassis

# 2. Установка зависимостей
echo "[2/8] Установка зависимостей..."
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    libi2c-dev \
    i2c-tools \
    python3 \
    python3-pip \
    git

# 3. Включение I2C интерфейса
echo "[3/8] Настройка I2C..."
if ! grep -q "dtparam=i2c_arm=on" /boot/config.txt 2>/dev/null; then
    echo "dtparam=i2c_arm=on" | sudo tee -a /boot/config.txt
    echo "I2C включен. Требуется перезагрузка."
fi

# 4. Копирование файлов
echo "[4/8] Копирование файлов проекта..."
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cp -r "$PROJECT_DIR"/* ~/robo_chassis/
cd ~/robo_chassis

# 5. Сборка проекта
echo "[5/8] Сборка проекта..."
mkdir -p build
cd build
cmake ..
make -j4
cd ..

# 6. Настройка прав доступа к устройствам
echo "[6/8] Настройка прав доступа..."
cat << 'EOF' | sudo tee /etc/udev/rules.d/99-robochassis.rules
# Разрешить пользователю pi доступ к USB UART и I2C
KERNEL=="ttyUSB*", MODE="0666"
KERNEL=="i2c-*", MODE="0666"
EOF
sudo udevadm control --reload-rules

# 7. Установка systemd сервиса
echo "[7/8] Установка systemd сервиса..."
sudo cp deploy/robo-chassis.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable robo-chassis

# 8. Добавление пользователя в группы
echo "[8/8] Добавление пользователя в группы..."
sudo usermod -aG dialout,i2c $USER

echo ""
echo "=== Установка завершена ==="
echo ""
echo "Доступные команды:"
echo "  sudo systemctl start robo-chassis    - Запуск сервиса"
echo "  sudo systemctl stop robo-chassis     - Остановка сервиса"
echo "  sudo systemctl status robo-chassis   - Проверка статуса"
echo "  journalctl -u robo-chassis -f        - Просмотр логов"
echo ""
echo "Для применения изменений групп требуется перезайти в систему."
echo "Если I2C был только что включен, требуется перезагрузка:"
echo "  sudo reboot"
echo ""
