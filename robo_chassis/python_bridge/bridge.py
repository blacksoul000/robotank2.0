import asyncio
import websockets
import aiohttp
from aiohttp import web
import json
import socket
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("Bridge")

# Конфигурация
TCP_HOST = '127.0.0.1'
TCP_PORT = 5555
HTTP_PORT = 8080
WS_PORT = 8765

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

async def handle_websocket(websocket, path=None):
    """Обработка WebSocket соединений с браузером"""
    connected_clients.add(websocket)
    logger.info(f"Браузер подключен. Всего клиентов: {len(connected_clients)}")
    
    try:
        async for message in websocket:
            try:
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
        logger.info(f"Браузер отключен. Осталось клиентов: {len(connected_clients)}")

async def telemetry_sender():
    """Периодическая опрос телеметрии (в реальной системе C++ сам пушит данные)"""
    # В данной архитектуре C++ не пушит телеметрию обратно через TCP
    # Телеметрия должна приходить отдельным каналом или запрашиваться
    # Для упрощения эмулируем получение телеметрии
    while True:
        await asyncio.sleep(0.1)  # 10 Hz
        
        # Эмуляция телеметрии (в реальности нужно читать ответ от C++)
        telemetry = {
            "type": "TELEMETRY",
            "battery": 12.4,
            "roll": 0.0,
            "pitch": 0.0,
            "turret_angle": 0.0,
            "signal_quality": 100
        }
        
        # Рассылка всем подключенным браузерам
        if connected_clients:
            msg = json.dumps(telemetry)
            await asyncio.gather(
                *[client.send(msg) for client in connected_clients],
                return_exceptions=True
            )

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
