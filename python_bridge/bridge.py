#!/usr/bin/env python3
"""
Python Bridge: TCP <-> WebSocket + HTTP Server
Связывает C++ приложение (через TCP localhost:5555) с браузером (через WebSocket).
Раздает HTML интерфейс через встроенный HTTP сервер.
"""

import asyncio
import json
import websockets
import socket
import threading
import time
import logging
import os
from aiohttp import web

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

# Глобальные переменные
websocket_clients = set()
tcp_socket = None
tcp_connected = False
tcp_reconnect_attempts = 0
MAX_RECONNECT_ATTEMPTS = 10  # Максимальное количество попыток переподключения
RECONNECT_DELAY_BASE = 2     # Базовая задержка между попытками (секунды)
RECONNECT_DELAY_MAX = 30     # Максимальная задержка (секунды)
loop = None

def connect_to_cpp():
    """Подключение к C++ приложению через TCP с автоматическим переподключением"""
    global tcp_socket, tcp_connected, loop, tcp_reconnect_attempts
    
    logger.info("Starting TCP client connection...")
    
    while True:
        try:
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp_socket.connect((TCP_HOST, TCP_PORT))
            tcp_socket.settimeout(1.0)
            tcp_connected = True
            tcp_reconnect_attempts = 0  # Сброс счётчика ошибок при успешном подключении
            logger.info(f"✓ Connected to C++ on {TCP_HOST}:{TCP_PORT}")
            print(f"✓ Подключено к C++ на {TCP_HOST}:{TCP_PORT}")
            
            # Чтение данных от C++
            buffer = ""
            while tcp_connected:
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
    """Раздача HTML интерфейса"""
    # Получаем хост из запроса для динамического определения адреса камеры
    host = request.host.split(':')[0]
    camera_url = f"http://{host}:{CAMERA_PORT}/stream/"
    
    html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>RoboChassis Control</title>
    <style>
        body {{ margin: 0; padding: 0; background: #1a1a1a; color: white; font-family: Arial; overflow: hidden; }}
        .container {{ display: flex; flex-direction: column; height: 100vh; }}
        .video-container {{ flex: 1; position: relative; background: #000; }}
        .video-container iframe {{ width: 100%; height: 100%; border: none; }}
        .controls {{ height: 220px; background: #2a2a2a; padding: 10px; display: flex; flex-direction: column; }}
        .controls-top {{ display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }}
        .buttons {{ display: flex; gap: 10px; }}
        .btn {{ padding: 15px 25px; font-size: 16px; border: none; border-radius: 8px; cursor: pointer; color: white; font-weight: bold; }}
        .btn-light {{ background: #ffd700; color: #000; }}
        .btn-light.active {{ background: #ffeb3b; box-shadow: 0 0 15px #ffd700; }}
        .btn-fire {{ background: #ff4444; }}
        .btn-fire.active {{ background: #ff6666; box-shadow: 0 0 15px #ff4444; }}
        .joysticks {{ display: flex; justify-content: space-around; align-items: center; flex: 1; }}
        .joystick {{ width: 120px; height: 120px; background: #444; border-radius: 50%; position: relative; touch-action: none; cursor: pointer; }}
        .stick {{ width: 50px; height: 50px; background: #ff6b6b; border-radius: 50%; position: absolute; top: 35px; left: 35px; pointer-events: none; }}
        .telemetry {{ position: absolute; top: 10px; left: 10px; background: rgba(0,0,0,0.7); padding: 10px; border-radius: 5px; font-size: 14px; z-index: 100; }}
        .telemetry div {{ margin: 5px 0; }}
        .logs {{ position: absolute; top: 10px; right: 10px; background: rgba(0,0,0,0.7); padding: 10px; border-radius: 5px; font-size: 12px; z-index: 100; width: 300px; max-height: 200px; overflow-y: auto; }}
        .log-entry {{ margin: 3px 0; font-family: monospace; }}
        .log-error {{ color: #ff6b6b; }}
        .log-warning {{ color: #ffd700; }}
        .log-info {{ color: #4ecdc4; }}
    </style>
</head>
<body>
    <div class="container">
        <div class="video-container">
            <iframe src="{camera_url}" allowfullscreen></iframe>
            <div class="telemetry">
                <div>🔋 Батарея: <span id="battery">--</span>%</div>
                <div>📐 Крен: <span id="roll">--</span>°</div>
                <div>📐 Тангаж: <span id="pitch">--</span>°</div>
                <div>🎯 Башня: <span id="turret">--</span>°</div>
                <div>🌡️ CPU: <span id="cpu_temp">--</span>°C</div>
                <div>💾 RAM: <span id="ram_usage">--</span>%</div>
                <div>📶 Сигнал: <span id="signal">--</span>%</div>
            </div>
            <div class="logs" id="logs">
                <div style="font-weight: bold; margin-bottom: 5px;">📋 Логи:</div>
                <div id="log-entries"></div>
            </div>
        </div>
        <div class="controls">
            <div class="controls-top">
                <div class="buttons">
                    <button class="btn btn-light" id="lightBtn" onclick="toggleLight()">💡 Свет</button>
                    <button class="btn btn-fire" id="fireBtn" onclick="toggleFire()">🔥 Огонь</button>
                </div>
            </div>
            <div class="joysticks">
                <div class="joystick" id="leftJoystick"><div class="stick"></div></div>
                <div class="joystick" id="rightJoystick"><div class="stick"></div></div>
            </div>
        </div>
    </div>

    <script>
        let ws;
        const leftStick = document.querySelector('#leftJoystick .stick');
        const rightStick = document.querySelector('#rightJoystick .stick');
        let leftPos = {{ x: 0, y: 0 }};
        let rightPos = {{ x: 0, y: 0 }};
        let lightOn = false;
        let fireOn = false;
        const maxLogs = 50;

        function connect() {{
            ws = new WebSocket('ws://' + window.location.hostname + ':8765');
            ws.onopen = () => {{
                console.log('✓ Connected to server');
                addLog('Connected to server', 'info');
            }};
            ws.onclose = () => {{
                console.log('✗ Disconnected, reconnecting...');
                addLog('Disconnected, reconnecting...', 'warning');
                setTimeout(connect, 2000);
            }};
            ws.onerror = (err) => {{
                console.error('WebSocket error:', err);
                addLog('WebSocket error', 'error');
            }};
            ws.onmessage = (event) => {{
                const data = JSON.parse(event.data);
                if (data.type === 'TELEMETRY') {{
                    document.getElementById('battery').textContent = data.battery !== undefined ? data.battery : '--';
                    document.getElementById('roll').textContent = data.roll !== undefined ? data.roll.toFixed(1) : '--';
                    document.getElementById('pitch').textContent = data.pitch !== undefined ? data.pitch.toFixed(1) : '--';
                    document.getElementById('turret').textContent = data.turret_angle !== undefined ? data.turret_angle.toFixed(1) : '--';
                    document.getElementById('cpu_temp').textContent = data.cpu_temp !== undefined ? data.cpu_temp.toFixed(1) : '--';
                    document.getElementById('ram_usage').textContent = data.ram_usage !== undefined ? data.ram_usage.toFixed(1) : '--';
                    document.getElementById('signal').textContent = data.signal !== undefined ? data.signal : '--';
                }}
                if (data.type === 'LOG') {{
                    addLog(data.message, data.level || 'info');
                }}
            }};
        }}

        function addLog(message, level) {{
            const logEntries = document.getElementById('log-entries');
            const entry = document.createElement('div');
            entry.className = 'log-entry log-' + level;
            const time = new Date().toLocaleTimeString();
            entry.textContent = `[${{time}}] ${{message}}`;
            logEntries.appendChild(entry);
            
            // Keep only last maxLogs entries
            while (logEntries.children.length > maxLogs) {{
                logEntries.removeChild(logEntries.firstChild);
            }}
            // Auto-scroll to bottom
            const logsContainer = document.getElementById('logs');
            logsContainer.scrollTop = logsContainer.scrollHeight;
        }}

        function sendCommand() {{
            const cmd = {{
                type: 'COMMAND',
                left: leftPos,
                right: rightPos
            }};
            if (ws.readyState === WebSocket.OPEN) {{
                ws.send(JSON.stringify(cmd));
            }}
        }}

        function sendAction(action, state) {{
            const cmd = {{
                type: 'ACTION',
                action: action,
                state: state
            }};
            if (ws.readyState === WebSocket.OPEN) {{
                ws.send(JSON.stringify(cmd));
            }}
        }}

        function toggleLight() {{
            lightOn = !lightOn;
            document.getElementById('lightBtn').classList.toggle('active', lightOn);
            sendAction('light', lightOn);
            addLog('Light ' + (lightOn ? 'ON' : 'OFF'), 'info');
        }}

        function toggleFire() {{
            fireOn = !fireOn;
            document.getElementById('fireBtn').classList.toggle('active', fireOn);
            sendAction('fire', fireOn);
            addLog('Fire ' + (fireOn ? 'ON' : 'OFF'), 'warning');
        }}

        function setupJoystick(element, stick, callback) {{
            let isDragging = false;
            const maxDist = 35;
            const rect = element.getBoundingClientRect();
            const centerX = rect.left + rect.width / 2;
            const centerY = rect.top + rect.height / 2;

            function handleStart(clientX, clientY) {{
                isDragging = true;
                handleMove(clientX, clientY);
            }}

            function handleMove(clientX, clientY) {{
                if (!isDragging) return;
                
                let dx = clientX - centerX;
                let dy = clientY - centerY;
                const dist = Math.sqrt(dx * dx + dy * dy);
                
                if (dist > maxDist) {{
                    dx = (dx / dist) * maxDist;
                    dy = (dy / dist) * maxDist;
                }}
                
                stick.style.transform = `translate(${{dx}}px, ${{dy}}px)`;
                callback({{ x: dx / maxDist, y: dy / maxDist }});
            }}

            function handleEnd() {{
                isDragging = false;
                stick.style.transform = 'translate(0, 0)';
                callback({{ x: 0, y: 0 }});
            }}

            // Mouse events
            element.addEventListener('mousedown', (e) => {{
                e.preventDefault();
                handleStart(e.clientX, e.clientY);
            }});

            document.addEventListener('mousemove', (e) => {{
                handleMove(e.clientX, e.clientY);
            }});

            document.addEventListener('mouseup', () => {{
                handleEnd();
            }});

            // Touch events
            element.addEventListener('touchstart', (e) => {{
                e.preventDefault();
                const touch = e.touches[0];
                handleStart(touch.clientX, touch.clientY);
            }});

            element.addEventListener('touchmove', (e) => {{
                e.preventDefault();
                const touch = e.touches[0];
                handleMove(touch.clientX, touch.clientY);
            }});

            element.addEventListener('touchend', (e) => {{
                e.preventDefault();
                handleEnd();
            }});
        }}

        setupJoystick(document.getElementById('leftJoystick'), leftStick, (pos) => {{
            leftPos = pos;
            sendCommand();
        }});

        setupJoystick(document.getElementById('rightJoystick'), rightStick, (pos) => {{
            rightPos = pos;
            sendCommand();
        }});

        // Initial log message
        addLog('UI initialized', 'info');
        connect();
    </script>
</body>
</html>
    """
    return web.Response(text=html_content, content_type='text/html')

async def on_shutdown(app):
    """Очистка при остановке сервера"""
    for ws in list(websocket_clients):
        await ws.close()
    websocket_clients.clear()

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
    global loop
    loop = asyncio.get_event_loop()
    
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
    tcp_thread = threading.Thread(target=run_tcp_thread, daemon=True)
    tcp_thread.start()
    
    # Запуск HTTP/WebSocket сервера
    app = await init_app()
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', HTTP_PORT)
    await site.start()
    
    logger.info(f"✓ Server started. Open in browser: http://<IP_RPI>:{HTTP_PORT}")
    print(f"✓ Сервер запущен. Откройте в браузере: http://<IP_RPI>:{HTTP_PORT}")
    
    # Бесконечное ожидание
    while True:
        await asyncio.sleep(3600)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("🛑 Stopping Python Bridge...")
        print("\n🛑 Остановка Python Bridge...")
        if tcp_socket:
            tcp_socket.close()
