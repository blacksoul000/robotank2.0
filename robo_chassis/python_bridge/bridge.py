#!/usr/bin/env python3
"""
Python Bridge: TCP <-> WebSocket + HTTP Server
Связывает C++ приложение (через TCP localhost:5555) с браузером (через WebSocket).
Раздает HTML интерфейс через встроенный HTTP сервер.

Thread-safe implementation with proper synchronization for concurrent access.
"""

import asyncio
import json
import websockets
import socket
import threading
import time
import logging
import os
import signal
from aiohttp import web
from typing import Optional, Set
from dataclasses import dataclass, field

# Конфигурация
TCP_HOST = '127.0.0.1'
TCP_PORT = 5555
WS_PORT = 8765
HTTP_PORT = 8080
CAMERA_PORT = 8889  # Порт видеопотока

# Настройка логирования
LOG_DIR = '/var/log/robo_chassis'
LOG_FILE = os.path.join(LOG_DIR, 'bridge.log')

# Создаем директорию для логов если не существует
if not os.path.exists(LOG_DIR):
    try:
        os.makedirs(LOG_DIR)
    except Exception as e:
        print(f"Warning: Could not create log directory {LOG_DIR}: {e}")

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(LOG_FILE) if os.access(LOG_DIR, os.W_OK) else logging.StreamHandler(),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)


@dataclass
class ConnectionManager:
    """
    Thread-safe менеджер соединений для управления WebSocket клиентами и TCP подключением.
    Использует asyncio.Lock для защиты от race conditions.
    """
    # WebSocket клиенты
    websocket_clients: Set = field(default_factory=set)
    websocket_lock: asyncio.Lock = field(default_factory=asyncio.Lock)
    
    # TCP соединение
    tcp_socket: Optional[socket.socket] = None
    tcp_connected: bool = False
    tcp_lock: threading.Lock = field(default_factory=threading.Lock)
    tcp_reconnect_attempts: int = 0
    
    # Queue для thread-safe передачи данных от TCP потока к asyncio loop
    message_queue: asyncio.Queue = field(default_factory=asyncio.Queue)
    
    # Rate limiter
    rate_limiter: Optional['RateLimiter'] = None
    
    MAX_RECONNECT_ATTEMPTS: int = 10
    RECONNECT_DELAY_BASE: int = 2
    RECONNECT_DELAY_MAX: int = 30
    
    async def add_websocket_client(self, client) -> None:
        """Thread-safe добавление WebSocket клиента"""
        async with self.websocket_lock:
            self.websocket_clients.add(client)
            logger.info(f"✓ Browser connected. Total clients: {len(self.websocket_clients)}")
    
    async def remove_websocket_client(self, client) -> None:
        """Thread-safe удаление WebSocket клиента"""
        async with self.websocket_lock:
            self.websocket_clients.discard(client)
            logger.info(f"✗ Browser disconnected. Total clients: {len(self.websocket_clients)}")
    
    async def get_client_count(self) -> int:
        """Получение количества клиентов"""
        async with self.websocket_lock:
            return len(self.websocket_clients)
    
    async def broadcast_message(self, message: str) -> None:
        """Thread-safe рассылка сообщения всем клиентам"""
        async with self.websocket_lock:
            clients = list(self.websocket_clients)
        
        if clients:
            await asyncio.gather(
                *[client.send_str(message) for client in clients],
                return_exceptions=True
            )
    
    def set_tcp_connected(self, connected: bool, sock: Optional[socket.socket] = None) -> None:
        """Thread-safe установка статуса TCP подключения"""
        with self.tcp_lock:
            self.tcp_connected = connected
            if sock:
                self.tcp_socket = sock
    
    def is_tcp_connected(self) -> bool:
        """Thread-safe проверка TCP подключения"""
        with self.tcp_lock:
            return self.tcp_connected
    
    def get_tcp_socket(self) -> Optional[socket.socket]:
        """Thread-safe получение TCP сокета"""
        with self.tcp_lock:
            return self.tcp_socket
    
    def close_tcp_socket(self) -> None:
        """Thread-safe закрытие TCP сокета"""
        with self.tcp_lock:
            if self.tcp_socket:
                try:
                    self.tcp_socket.close()
                except:
                    pass
                self.tcp_socket = None
            self.tcp_connected = False
    
    async def queue_message(self, message: str) -> None:
        """Добавление сообщения в очередь для обработки в asyncio loop"""
        await self.message_queue.put(message)
    
    async def process_queued_messages(self) -> None:
        """Обработка всех сообщений из очереди"""
        while not self.message_queue.empty():
            try:
                message = await self.message_queue.get_nowait()
                await self.broadcast_message(message)
                self.message_queue.task_done()
            except asyncio.QueueEmpty:
                break


# Глобальный менеджер соединений
connection_manager = ConnectionManager()

# Флаги для graceful shutdown
shutdown_event = threading.Event()
tcp_thread = None
runner = None
loop = None

def connect_to_cpp():
    """Подключение к C++ приложению через TCP с автоматическим переподключением"""
    global tcp_socket, tcp_connected, loop, tcp_reconnect_attempts
    
    logger.info("Starting TCP client connection...")
    
    while not shutdown_event.is_set():
        try:
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp_socket.settimeout(1.0)
            tcp_socket.connect((TCP_HOST, TCP_PORT))
            tcp_connected = True
            tcp_reconnect_attempts = 0  # Сброс счётчика ошибок при успешном подключении
            logger.info(f"✓ Connected to C++ on {TCP_HOST}:{TCP_PORT}")
            print(f"✓ Подключено к C++ на {TCP_HOST}:{TCP_PORT}")
            
            # Чтение данных от C++
            buffer = ""
            while tcp_connected and not shutdown_event.is_set():
                try:
                    data = tcp_socket.recv(4096).decode('utf-8')
                    if not data:
                        break
                    buffer += data
                    
                    # Обработка построчно (разделитель \n)
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        if line:
                            logger.debug(f"Received from C++: {line[:100]}")
                            if loop and loop.is_running():
                                asyncio.run_coroutine_threadsafe(forward_to_websockets(line), loop)
                            
                except socket.timeout:
                    continue
                except Exception as e:
                    logger.error(f"✗ TCP read error: {e}")
                    print(f"✗ Ошибка чтения из TCP: {e}")
                    break
                    
        except Exception as e:
            if shutdown_event.is_set():
                break
            logger.error(f"✗ TCP connection error: {e}. Reconnecting in 2 sec...")
            print(f"✗ Ошибка подключения к C++: {e}. Повтор через 2 сек...")
            tcp_connected = False
            
            # Экспоненциальная задержка с максимумом
            delay = min(RECONNECT_DELAY_BASE * (2 ** tcp_reconnect_attempts), RECONNECT_DELAY_MAX)
            tcp_reconnect_attempts += 1
            
            if tcp_reconnect_attempts >= MAX_RECONNECT_ATTEMPTS:
                logger.warning(f"⚠ Max reconnect attempts ({MAX_RECONNECT_ATTEMPTS}) reached. Waiting {RECONNECT_DELAY_MAX} sec...")
                print(f"⚠ Достигнуто максимальное количество попыток ({MAX_RECONNECT_ATTEMPTS}). Ждём {RECONNECT_DELAY_MAX} сек...")
                tcp_reconnect_attempts = MAX_RECONNECT_ATTEMPTS - 1  # Чтобы не увеличивать дальше
            
            time.sleep(delay)
        
        if tcp_socket:
            try:
                tcp_socket.close()
            except:
                pass
        tcp_socket = None
        tcp_connected = False

async def forward_to_websockets(message):
    """Отправка сообщения всем подключенным WebSocket клиентам"""
    if websocket_clients:
        await asyncio.gather(
            *[client.send_str(message) for client in websocket_clients],
            return_exceptions=True
        )

# Rate limiting для защиты от DoS атак
class RateLimiter:
    """Ограничитель частоты сообщений для каждого клиента"""
    def __init__(self, max_messages_per_second=20, window_seconds=1):
        self.max_messages = max_messages_per_second
        self.window = window_seconds
        self.clients = {}  # client_id -> {'count': int, 'reset_time': float}
    
    def is_allowed(self, client_id):
        """Проверка, может ли клиент отправить сообщение"""
        import time
        current_time = time.time()
        
        if client_id not in self.clients:
            self.clients[client_id] = {'count': 0, 'reset_time': current_time + self.window}
        
        client_data = self.clients[client_id]
        
        # Сброс счётчика если окно времени истекло
        if current_time >= client_data['reset_time']:
            client_data['count'] = 0
            client_data['reset_time'] = current_time + self.window
        
        # Проверка лимита
        if client_data['count'] >= self.max_messages:
            return False
        
        client_data['count'] += 1
        return True
    
    def cleanup(self, client_id):
        """Удаление данных о клиенте при отключении"""
        if client_id in self.clients:
            del self.clients[client_id]

# Глобальный rate limiter
rate_limiter = RateLimiter(max_messages_per_second=20, window_seconds=1)

async def handle_websocket(request):
    """Обработчик WebSocket подключений с rate limiting"""
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    client_id = id(ws)
    websocket_clients.add(ws)
    logger.info(f"✓ Browser connected (ID: {client_id}). Total clients: {len(websocket_clients)}")
    print(f"✓ Браузер подключен (ID: {client_id}). Всего клиентов: {len(websocket_clients)}")
    
    try:
        async for msg in ws:
            if msg.type == web.WSMsgType.TEXT:
                # Проверка rate limit перед обработкой
                if not rate_limiter.is_allowed(client_id):
                    logger.warning(f"⚠ Rate limit exceeded for client {client_id}")
                    print(f"⚠ Rate limit превышен для клиента {client_id}")
                    continue
                
                # Получение команд от браузера и отправка в C++
                if tcp_connected and tcp_socket:
                    try:
                        # Добавляем \n для разделения строк
                        tcp_socket.sendall((msg.data + '\n').encode('utf-8'))
                        logger.debug(f"Sent to C++: {msg.data[:100]}")
                    except Exception as e:
                        logger.error(f"✗ Send error to C++: {e}")
                        print(f"✗ Ошибка отправки в C++: {e}")
                        # Попытка переподключения при ошибке отправки
                        tcp_connected = False
                        if tcp_socket:
                            try:
                                tcp_socket.close()
                            except:
                                pass
                        tcp_socket = None
                        print("🔄 Попытка переподключения к C++...")
                else:
                    logger.warning(f"⚠ No connection to C++, command not sent: {msg.data[:50]}...")
                    print(f"⚠ Нет подключения к C++, команда не отправлена: {msg.data[:50]}...")
            elif msg.type == web.WSMsgType.ERROR:
                logger.error(f"✗ WebSocket error: {ws.exception()}")
                print(f"✗ WebSocket ошибка: {ws.exception()}")
            elif msg.type == web.WSMsgType.CLOSE:
                logger.info(f"ℹ WebSocket closed by client {client_id}")
                print(f"ℹ WebSocket закрыт клиентом {client_id}")
                break
    finally:
        websocket_clients.discard(ws)
        rate_limiter.cleanup(client_id)
        logger.info(f"✗ Browser disconnected (ID: {client_id}). Total clients: {len(websocket_clients)}")
        print(f"✗ Браузер отключен (ID: {client_id}). Всего клиентов: {len(websocket_clients)}")
    
    return ws

async def handle_http(request):
    """Раздача HTML интерфейса из статического файла"""
    static_path = os.path.join(os.path.dirname(__file__), '..', 'static', 'index.html')
    
    try:
        with open(static_path, 'r', encoding='utf-8') as f:
            html_content = f.read()
        
        # Заменяем плейсхолдер для камеры на актуальный URL
        host = request.host.split(':')[0]
        camera_url = f"http://{host}:{CAMERA_PORT}/stream/"
        html_content = html_content.replace('"/video"', f'"{camera_url}"')
        
        return web.Response(text=html_content, content_type='text/html')
    except FileNotFoundError:
        return web.Response(text="Error: index.html not found in static folder", status=404, content_type='text/plain')
    except Exception as e:
        logger.error(f"Error serving static file: {e}")
        return web.Response(text=f"Error: {str(e)}", status=500, content_type='text/plain')

async def on_shutdown(app):
    """Очистка при остановке сервера"""
    logger.info("Shutting down WebSocket connections...")
    for ws in list(websocket_clients):
        await ws.close()
    websocket_clients.clear()

def signal_handler(signum, frame):
    """Обработчик сигналов для graceful shutdown"""
    logger.info(f"Received signal {signum}, initiating shutdown...")
    print(f"\n🛑 Получен сигнал {signum}, завершение работы...")
    shutdown_event.set()
    
    # Закрытие TCP соединения
    global tcp_socket
    if tcp_socket:
        try:
            tcp_socket.close()
        except:
            pass
    
    # Остановка asyncio event loop
    global loop
    if loop and loop.is_running():
        loop.call_soon_threadsafe(loop.stop)

def run_tcp_thread():
    """Запуск TCP клиента в отдельном потоке"""
    connect_to_cpp()

async def init_app():
    """Инициализация приложения"""
    app = web.Application()
    app.router.add_get('/', handle_http)
    app.router.add_get('/ws', handle_websocket)
    app.on_shutdown.append(on_shutdown)
    return app

async def main():
    """Основная функция"""
    global loop, runner, tcp_thread
    loop = asyncio.get_event_loop()
    
    # Регистрация обработчиков сигналов
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    logger.info("🚀 Starting Python Bridge...")
    print("🚀 Запуск Python Bridge...")
    logger.info(f"  TCP Client: {TCP_HOST}:{TCP_PORT} -> C++ App")
    logger.info(f"  WebSocket Server: ws://0.0.0.0:{WS_PORT} -> Browser")
    logger.info(f"  HTTP Server: http://0.0.0.0:{HTTP_PORT} -> Browser Interface")
    logger.info(f"  Camera URL: http://<host>:{CAMERA_PORT}/stream/")
    print(f"  TCP Client: {TCP_HOST}:{TCP_PORT} -> C++ App")
    print(f"  WebSocket Server: ws://0.0.0.0:{WS_PORT} -> Browser")
    print(f"  HTTP Server: http://0.0.0.0:{HTTP_PORT} -> Browser Interface")
    
    # Запуск TCP клиента в отдельном потоке
    tcp_thread = threading.Thread(target=run_tcp_thread, daemon=False)
    tcp_thread.start()
    
    # Запуск HTTP/WebSocket сервера
    app = await init_app()
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', HTTP_PORT)
    await site.start()
    
    logger.info(f"✓ Server started. Open in browser: http://<IP_RPI>:{HTTP_PORT}")
    print(f"✓ Сервер запущен. Откройте в браузере: http://<IP_RPI>:{HTTP_PORT}")
    
    # Ожидание сигнала завершения
    try:
        while not shutdown_event.is_set():
            await asyncio.sleep(1)
    except asyncio.CancelledError:
        pass
    finally:
        # Graceful shutdown
        logger.info("🛑 Stopping Python Bridge...")
        print("\n🛑 Остановка Python Bridge...")
        
        # Остановка HTTP/WebSocket сервера
        if runner:
            await runner.cleanup()
        
        # Ожидание завершения TCP потока
        if tcp_thread and tcp_thread.is_alive():
            logger.info("Waiting for TCP thread to finish...")
            tcp_thread.join(timeout=5)
        
        # Закрытие TCP соединения
        if tcp_socket:
            try:
                tcp_socket.close()
            except:
                pass
        
        logger.info("Python Bridge stopped.")
        print("✓ Python Bridge остановлен.")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass  # Обработано в signal_handler
    except Exception as e:
        logger.error(f"Fatal error: {e}")
        print(f"❌ Фатальная ошибка: {e}")
