#!/usr/bin/env python3
"""
Python Bridge: TCP <-> WebSocket + HTTP Server
Связывает C++ приложение (через TCP localhost:5555) с браузером (через WebSocket).
Раздает HTML интерфейс через встроенный HTTP сервер.

Thread-safe implementation with proper synchronization for concurrent access.
All operations with shared state are protected by asyncio.Lock or threading.Lock.
Uses asyncio.Queue for thread-safe data transfer between TCP thread and asyncio loop.
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
from typing import Optional, Set, Dict, Any
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
    Использует asyncio.Lock для защиты от race conditions при доступе к websocket_clients.
    Все операции с общим состоянием обернуты в lock.
    
    Uses asyncio.Queue for thread-safe data transfer from TCP thread to asyncio loop.
    Replaces global variables with a proper manager class.
    """
    # WebSocket клиенты - защищены asyncio.Lock
    _websocket_clients: Set = field(default_factory=set, repr=False)
    _websocket_lock: asyncio.Lock = field(default=None, repr=False)
    
    # TCP соединение - защищено threading.Lock
    _tcp_socket: Optional[socket.socket] = field(default=None, repr=False)
    _tcp_connected: bool = field(default=False, repr=False)
    _tcp_lock: threading.Lock = field(default_factory=threading.Lock, repr=False)
    _tcp_reconnect_attempts: int = field(default=0, repr=False)
    
    # Queue для thread-safe передачи данных от TCP потока к asyncio loop
    _message_queue: asyncio.Queue = field(default=None, repr=False)
    
    # Rate limiter
    _rate_limiter: Optional['RateLimiter'] = field(default=None, repr=False)
    
    # Константы переподключения
    MAX_RECONNECT_ATTEMPTS: int = field(default=10, init=False)
    RECONNECT_DELAY_BASE: int = field(default=2, init=False)
    RECONNECT_DELAY_MAX: int = field(default=30, init=False)
    
    def __post_init__(self):
        """Инициализация полей после создания объекта"""
        if self._websocket_lock is None:
            object.__setattr__(self, '_websocket_lock', asyncio.Lock())
        if self._message_queue is None:
            object.__setattr__(self, '_message_queue', asyncio.Queue())
        if self._rate_limiter is None:
            object.__setattr__(self, '_rate_limiter', RateLimiter(max_messages_per_second=20, window_seconds=1))
        object.__setattr__(self, 'MAX_RECONNECT_ATTEMPTS', 10)
        object.__setattr__(self, 'RECONNECT_DELAY_BASE', 2)
        object.__setattr__(self, 'RECONNECT_DELAY_MAX', 30)
    
    @property
    def websocket_clients(self) -> Set:
        """Прямой доступ к клиентам только для чтения (для совместимости)"""
        return self._websocket_clients
    
    @property
    def websocket_lock(self) -> asyncio.Lock:
        """Lock для операций с websocket клиентами"""
        return self._websocket_lock
    
    @property
    def tcp_lock(self) -> threading.Lock:
        """Lock для операций с TCP соединением"""
        return self._tcp_lock
    
    @property
    def message_queue(self) -> asyncio.Queue:
        """Queue для thread-safe передачи сообщений"""
        return self._message_queue
    
    @property
    def rate_limiter(self) -> 'RateLimiter':
        """Rate limiter для защиты от DoS"""
        return self._rate_limiter
    
    async def add_websocket_client(self, client) -> None:
        """Thread-safe добавление WebSocket клиента"""
        async with self._websocket_lock:
            self._websocket_clients.add(client)
            logger.info(f"✓ Browser connected. Total clients: {len(self._websocket_clients)}")
    
    async def remove_websocket_client(self, client) -> None:
        """Thread-safe удаление WebSocket клиента"""
        async with self._websocket_lock:
            self._websocket_clients.discard(client)
            logger.info(f"✗ Browser disconnected. Total clients: {len(self._websocket_clients)}")
    
    async def get_client_count(self) -> int:
        """Получение количества клиентов"""
        async with self._websocket_lock:
            return len(self._websocket_clients)
    
    async def broadcast_message(self, message: str) -> None:
        """Thread-safe рассылка сообщения всем клиентам"""
        async with self._websocket_lock:
            clients = list(self._websocket_clients)
        
        if clients:
            await asyncio.gather(
                *[client.send_str(message) for client in clients],
                return_exceptions=True
            )
    
    def set_tcp_connected(self, connected: bool, sock: Optional[socket.socket] = None) -> None:
        """Thread-safe установка статуса TCP подключения"""
        with self._tcp_lock:
            self._tcp_connected = connected
            if sock:
                self._tcp_socket = sock
    
    def is_tcp_connected(self) -> bool:
        """Thread-safe проверка TCP подключения"""
        with self._tcp_lock:
            return self._tcp_connected
    
    def get_tcp_socket(self) -> Optional[socket.socket]:
        """Thread-safe получение TCP сокета"""
        with self._tcp_lock:
            return self._tcp_socket
    
    def close_tcp_socket(self) -> None:
        """Thread-safe закрытие TCP сокета"""
        with self._tcp_lock:
            if self._tcp_socket:
                try:
                    self._tcp_socket.close()
                except:
                    pass
                self._tcp_socket = None
            self._tcp_connected = False
    
    def get_tcp_reconnect_attempts(self) -> int:
        """Thread-safe получение счётчика попыток переподключения"""
        with self._tcp_lock:
            return self._tcp_reconnect_attempts
    
    def set_tcp_reconnect_attempts(self, attempts: int) -> None:
        """Thread-safe установка счётчика попыток переподключения"""
        with self._tcp_lock:
            self._tcp_reconnect_attempts = attempts
    
    def increment_tcp_reconnect_attempts(self) -> int:
        """Thread-safe инкремент счётчика попыток, возвращает новое значение"""
        with self._tcp_lock:
            self._tcp_reconnect_attempts += 1
            return self._tcp_reconnect_attempts
    
    async def queue_message(self, message: str) -> None:
        """Добавление сообщения в очередь для обработки в asyncio loop"""
        await self._message_queue.put(message)
    
    async def process_queued_messages(self) -> None:
        """Обработка всех сообщений из очереди"""
        while not self._message_queue.empty():
            try:
                message = await self._message_queue.get_nowait()
                await self.broadcast_message(message)
                self._message_queue.task_done()
            except asyncio.QueueEmpty:
                break
    
    async def cleanup(self) -> None:
        """Очистка всех соединений при shutdown"""
        # Закрываем все WebSocket подключения
        async with self._websocket_lock:
            for client in list(self._websocket_clients):
                try:
                    await client.close()
                except:
                    pass
            self._websocket_clients.clear()
        
        # Закрываем TCP соединение
        self.close_tcp_socket()
        
        # Очищаем очередь сообщений
        while not self._message_queue.empty():
            try:
                self._message_queue.get_nowait()
                self._message_queue.task_done()
            except asyncio.QueueEmpty:
                break


# Глобальный менеджер соединений - заменяет все глобальные переменные
connection_manager = ConnectionManager()

# Флаги для graceful shutdown
shutdown_event = threading.Event()
tcp_thread: Optional[threading.Thread] = None
runner = None
loop = None

def connect_to_cpp():
    """Подключение к C++ приложению через TCP с автоматическим переподключением"""
    global loop
    
    logger.info("Starting TCP client connection...")
    
    while not shutdown_event.is_set():
        try:
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp_socket.settimeout(1.0)
            tcp_socket.connect((TCP_HOST, TCP_PORT))
            connection_manager.set_tcp_connected(True, tcp_socket)
            connection_manager.set_tcp_reconnect_attempts(0)  # Сброс счётчика ошибок при успешном подключении
            logger.info(f"✓ Connected to C++ on {TCP_HOST}:{TCP_PORT}")
            print(f"✓ Подключено к C++ на {TCP_HOST}:{TCP_PORT}")
            
            # Чтение данных от C++
            buffer = ""
            while connection_manager.is_tcp_connected() and not shutdown_event.is_set():
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
                                # Используем очередь для thread-safe передачи
                                asyncio.run_coroutine_threadsafe(
                                    connection_manager.queue_message(line), loop
                                )
                            
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
            connection_manager.set_tcp_connected(False)
            
            # Экспоненциальная задержка с максимумом
            attempts = connection_manager.increment_tcp_reconnect_attempts()
            delay = min(connection_manager.RECONNECT_DELAY_BASE * (2 ** (attempts - 1)), 
                       connection_manager.RECONNECT_DELAY_MAX)
            
            if attempts >= connection_manager.MAX_RECONNECT_ATTEMPTS:
                logger.warning(f"⚠ Max reconnect attempts ({connection_manager.MAX_RECONNECT_ATTEMPTS}) reached. Waiting {connection_manager.RECONNECT_DELAY_MAX} sec...")
                print(f"⚠ Достигнуто максимальное количество попыток ({connection_manager.MAX_RECONNECT_ATTEMPTS}). Ждём {connection_manager.RECONNECT_DELAY_MAX} сек...")
                connection_manager.set_tcp_reconnect_attempts(connection_manager.MAX_RECONNECT_ATTEMPTS - 1)
            
            time.sleep(delay)
        
        if tcp_socket:
            try:
                tcp_socket.close()
            except:
                pass
        connection_manager.close_tcp_socket()

async def forward_to_websockets(message):
    """Отправка сообщения всем подключенным WebSocket клиентам через менеджер соединений"""
    await connection_manager.broadcast_message(message)

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

# Глобальный rate limiter - больше не используется, доступен через connection_manager.rate_limiter
# rate_limiter = RateLimiter(max_messages_per_second=20, window_seconds=1)

# Константы переподключения - доступны через connection_manager
RECONNECT_DELAY_BASE = 2
RECONNECT_DELAY_MAX = 30
MAX_RECONNECT_ATTEMPTS = 10

async def handle_websocket(request):
    """Обработчик WebSocket подключений с rate limiting"""
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    client_id = id(ws)
    await connection_manager.add_websocket_client(ws)
    logger.info(f"✓ Browser connected (ID: {client_id}). Total clients: {await connection_manager.get_client_count()}")
    print(f"✓ Браузер подключен (ID: {client_id}). Всего клиентов: {await connection_manager.get_client_count()}")
    
    try:
        async for msg in ws:
            if msg.type == web.WSMsgType.TEXT:
                # Проверка rate limit перед обработкой
                if not connection_manager.rate_limiter.is_allowed(client_id):
                    logger.warning(f"⚠ Rate limit exceeded for client {client_id}")
                    print(f"⚠ Rate limit превышен для клиента {client_id}")
                    continue
                
                # Получение команд от браузера и отправка в C++
                tcp_socket = connection_manager.get_tcp_socket()
                if connection_manager.is_tcp_connected() and tcp_socket:
                    try:
                        # Добавляем \n для разделения строк
                        tcp_socket.sendall((msg.data + '\n').encode('utf-8'))
                        logger.debug(f"Sent to C++: {msg.data[:100]}")
                    except Exception as e:
                        logger.error(f"✗ Send error to C++: {e}")
                        print(f"✗ Ошибка отправки в C++: {e}")
                        # Попытка переподключения при ошибке отправки
                        connection_manager.set_tcp_connected(False)
                        if tcp_socket:
                            try:
                                tcp_socket.close()
                            except:
                                pass
                        connection_manager.close_tcp_socket()
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
        await connection_manager.remove_websocket_client(ws)
        connection_manager.rate_limiter.cleanup(client_id)
        logger.info(f"✗ Browser disconnected (ID: {client_id}). Total clients: {await connection_manager.get_client_count()}")
        print(f"✗ Браузер отключен (ID: {client_id}). Всего клиентов: {await connection_manager.get_client_count()}")
    
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
    await connection_manager.cleanup()

def signal_handler(signum, frame):
    """Обработчик сигналов для graceful shutdown"""
    logger.info(f"Received signal {signum}, initiating shutdown...")
    print(f"\n🛑 Получен сигнал {signum}, завершение работы...")
    shutdown_event.set()
    
    # Закрытие TCP соединения через менеджер
    connection_manager.close_tcp_socket()
    
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
        
        # Закрытие TCP соединения через менеджер
        connection_manager.close_tcp_socket()
        
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
