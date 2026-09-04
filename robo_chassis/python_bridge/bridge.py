import asyncio
import websockets
import aiohttp
from aiohttp import web
import json
import socket
import logging
import time
from collections import defaultdict

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("Bridge")

# Конфигурация
TCP_HOST = '127.0.0.1'
TCP_PORT = 5555
HTTP_PORT = 8080
WS_PORT = 8765
RATE_LIMIT_MESSAGES_PER_SEC = 20  # Максимум сообщений в секунду от клиента

class RateLimiter:
    """Ограничитель частоты сообщений для защиты от DoS"""
    def __init__(self, max_messages_per_sec=RATE_LIMIT_MESSAGES_PER_SEC):
        self.max_messages = max_messages_per_sec
        self.client_timestamps = defaultdict(list)
        
    def is_allowed(self, client_id):
        """Проверяет, можно ли отправить сообщение этому клиенту"""
        now = time.time()
        # Очищаем старые временные метки (старше 1 секунды)
        self.client_timestamps[client_id] = [
            ts for ts in self.client_timestamps[client_id] 
            if now - ts < 1.0
        ]
        
        # Проверяем лимит
        if len(self.client_timestamps[client_id]) >= self.max_messages:
            return False
            
        # Добавляем текущую временную метку
        self.client_timestamps[client_id].append(now)
        return True
    
    def get_remaining(self, client_id):
        """Возвращает количество оставшихся сообщений в текущей секунде"""
        now = time.time()
        self.client_timestamps[client_id] = [
            ts for ts in self.client_timestamps[client_id] 
            if now - ts < 1.0
        ]
        return max(0, self.max_messages - len(self.client_timestamps[client_id]))

class TcpClient:
    """Клиент для связи с C++ ядром по TCP"""
    def __init__(self):
        self.reader = None
        self.writer = None
        self.connected = False
        
    async def connect(self):
        try:
            self.reader, self.writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)
            self.connected = True
            logger.info(f"Подключено к C++ ядру на {TCP_HOST}:{TCP_PORT}")
            return True
        except Exception as e:
            logger.error(f"Ошибка подключения к C++: {e}")
            self.connected = False
            return False
            
    async def send_command(self, command_json):
        if not self.connected or not self.writer:
            if not await self.connect():
                return False
        try:
            # Отправляем команду с завершающим \n
            self.writer.write((command_json + '\n').encode())
            await self.writer.drain()
            return True
        except Exception as e:
            logger.error(f"Ошибка отправки команды: {e}")
            self.connected = False
            return False
            
    async def close(self):
        if self.writer:
            self.writer.close()
            await self.writer.wait_closed()
        self.connected = False

# Глобальные объекты
tcp_client = TcpClient()
connected_clients = set()  # WebSocket клиенты
rate_limiter = RateLimiter(RATE_LIMIT_MESSAGES_PER_SEC)  # Ограничитель частоты

async def handle_websocket(websocket, path=None):
    """Обработка WebSocket соединений с браузером"""
    # Используем ID клиента для rate limiting
    client_id = id(websocket)
    connected_clients.add(websocket)
    logger.info(f"Браузер подключен. Всего клиентов: {len(connected_clients)}")
    
    try:
        async for message in websocket:
            try:
                # Проверка rate limiting
                if not rate_limiter.is_allowed(client_id):
                    remaining = rate_limiter.get_remaining(client_id)
                    if remaining == 0:
                        logger.warning(f"Rate limit превышен для клиента {client_id}")
                        # Отправляем предупреждение клиенту
                        await websocket.send(json.dumps({
                            "type": "WARNING",
                            "message": f"Rate limit exceeded. Max {RATE_LIMIT_MESSAGES_PER_SEC} msg/sec"
                        }))
                    continue
                
                data = json.loads(message)
                
                # Если это команда управления - отправляем в C++
                if data.get('type') == 'COMMAND':
                    # Удаляем тип сообщения для отправки в C++
                    cmd_data = {k: v for k, v in data.items() if k != 'type'}
                    cmd_json = json.dumps(cmd_data)
                    
                    if await tcp_client.send_command(cmd_json):
                        logger.debug(f"Команда отправлена: {cmd_json}")
                    else:
                        # Попытка переподключения
                        await tcp_client.connect()
                        
            except json.JSONDecodeError:
                logger.warning(f"Неверный JSON от клиента: {message}")
            except Exception as e:
                logger.error(f"Ошибка обработки сообщения: {e}")
                
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        connected_clients.remove(websocket)
        # Очищаем данные rate limiter при отключении
        if client_id in rate_limiter.client_timestamps:
            del rate_limiter.client_timestamps[client_id]
        logger.info(f"Браузер отключен. Осталось клиентов: {len(connected_clients)}")

async def telemetry_sender():
    """Периодическая отправка телеметрии от C++ ядра"""
    # В данной архитектуре C++ ядро отправляет телеметрию через TCP
    # Читаем данные и транслируем в WebSocket
    
    while True:
        await asyncio.sleep(0.5)  # 2 Hz - достаточно для системной телеметрии
        
        if not tcp_client.connected or not tcp_client.reader:
            continue
        
        try:
            # Запрашиваем телеметрию у C++ ядра (команда GET_STATS)
            tcp_client.writer.write(b'{"cmd":"GET_STATS"}\n')
            await tcp_client.writer.drain()
            
            # Читаем ответ (таймаут 1 секунда)
            try:
                response = await asyncio.wait_for(
                    tcp_client.reader.readline(), 
                    timeout=1.0
                )
                if response:
                    data = json.loads(response.decode())
                    
                    # Формируем расширенную телеметрию
                    telemetry = {
                        "type": "TELEMETRY",
                        "battery": data.get('battery', 0.0),
                        "roll": data.get('roll', 0.0),
                        "pitch": data.get('pitch', 0.0),
                        "turret_angle": data.get('turret_angle', 0.0),
                        "signal_quality": 100,
                        # Системная телеметрия от SystemMonitor
                        "cpu_temp": data.get('cpu_temp', 0.0),
                        "memory_percent": data.get('memory_percent', 0.0),
                        "cpu_usage": data.get('cpu_usage', 0.0)
                    }
                    
                    # Рассылка всем подключенным браузерам
                    if connected_clients:
                        msg = json.dumps(telemetry)
                        await asyncio.gather(
                            *[client.send(msg) for client in connected_clients],
                            return_exceptions=True
                        )
            except asyncio.TimeoutError:
                pass  # Нет ответа от C++, используем старые данные
            except json.JSONDecodeError:
                pass  # Неверный формат
                
        except Exception as e:
            logger.error(f"Ошибка получения телеметрии: {e}")
            await tcp_client.connect()  # Попытка переподключения

async def http_handler(request):
    """Раздача статических файлов (веб-интерфейс)"""
    file_path = request.path[1:]  # Убираем начальный /
    if file_path == '' or file_path == '/':
        file_path = 'index.html'
    
    static_dir = 'static'
    full_path = f"{static_dir}/{file_path}"
    
    try:
        with open(full_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        content_type = 'text/html'
        if file_path.endswith('.css'):
            content_type = 'text/css'
        elif file_path.endswith('.js'):
            content_type = 'application/javascript'
            
        return web.Response(text=content, content_type=content_type)
    except FileNotFoundError:
        return web.Response(text="404 Not Found", status=404)

async def main():
    # Подключение к C++ ядру
    await tcp_client.connect()
    
    # Запуск HTTP сервера
    app = web.Application()
    app.router.add_get('/', http_handler)
    app.router.add_get('/{file}', http_handler)
    
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', HTTP_PORT)
    await site.start()
    logger.info(f"HTTP сервер запущен на порту {HTTP_PORT}")
    
    # Запуск WebSocket сервера
    ws_server = await websockets.serve(handle_websocket, "0.0.0.0", WS_PORT)
    logger.info(f"WebSocket сервер запущен на порту {WS_PORT}")
    
    # Запуск отправителя телеметрии
    telemetry_task = asyncio.create_task(telemetry_sender())
    
    logger.info("=== Python Bridge готов ===")
    logger.info(f"Откройте в браузере: http://<IP_RPI>:{HTTP_PORT}")
    
    # Ожидание завершения
    try:
        await asyncio.Future()  # Бесконечное ожидание
    except asyncio.CancelledError:
        pass
    finally:
        telemetry_task.cancel()
        await tcp_client.close()
        ws_server.close()
        await ws_server.wait_closed()
        await runner.cleanup()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Остановка по Ctrl+C")
