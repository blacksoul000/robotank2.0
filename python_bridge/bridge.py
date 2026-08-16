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
from aiohttp import web

# Конфигурация
TCP_HOST = '127.0.0.1'
TCP_PORT = 5555
WS_PORT = 8765
HTTP_PORT = 8080

# Глобальные переменные
websocket_clients = set()
tcp_socket = None
tcp_connected = False
loop = None

def connect_to_cpp():
    """Подключение к C++ приложению через TCP"""
    global tcp_socket, tcp_connected, loop
    
    while True:
        try:
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp_socket.connect((TCP_HOST, TCP_PORT))
            tcp_socket.settimeout(1.0)
            tcp_connected = True
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
                            if loop and loop.is_running():
                                asyncio.run_coroutine_threadsafe(forward_to_websockets(line), loop)
                            
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"✗ Ошибка чтения из TCP: {e}")
                    break
                    
        except Exception as e:
            print(f"✗ Ошибка подключения к C++: {e}. Повтор через 2 сек...")
            tcp_connected = False
            time.sleep(2)
        
        if tcp_socket:
            try:
                tcp_socket.close()
            except:
                pass
        tcp_socket = None
        tcp_connected = False
        time.sleep(2)

async def forward_to_websockets(message):
    """Отправка сообщения всем подключенным WebSocket клиентам"""
    if websocket_clients:
        await asyncio.gather(
            *[client.send_str(message) for client in websocket_clients],
            return_exceptions=True
        )

async def handle_websocket(request):
    """Обработчик WebSocket подключений"""
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    websocket_clients.add(ws)
    print(f"✓ Браузер подключен. Всего клиентов: {len(websocket_clients)}")
    
    try:
        async for msg in ws:
            if msg.type == web.WSMsgType.TEXT:
                # Получение команд от браузера и отправка в C++
                if tcp_connected and tcp_socket:
                    try:
                        # Добавляем \n для разделения строк
                        tcp_socket.sendall((msg.data + '\n').encode('utf-8'))
                    except Exception as e:
                        print(f"✗ Ошибка отправки в C++: {e}")
                        tcp_connected = False
            elif msg.type == web.WSMsgType.ERROR:
                print(f"✗ WebSocket ошибка: {ws.exception()}")
    finally:
        websocket_clients.remove(ws)
        print(f"✗ Браузер отключен. Всего клиентов: {len(websocket_clients)}")
    
    return ws

async def handle_http(request):
    """Раздача HTML интерфейса"""
    html_content = """
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>RoboChassis Control</title>
    <style>
        body { margin: 0; padding: 0; background: #1a1a1a; color: white; font-family: Arial; overflow: hidden; }
        .container { display: flex; flex-direction: column; height: 100vh; }
        .video-container { flex: 1; position: relative; background: #000; }
        .video-container iframe { width: 100%; height: 100%; border: none; }
        .controls { height: 200px; background: #2a2a2a; padding: 10px; display: flex; justify-content: space-around; align-items: center; }
        .joystick { width: 120px; height: 120px; background: #444; border-radius: 50%; position: relative; touch-action: none; }
        .stick { width: 50px; height: 50px; background: #ff6b6b; border-radius: 50%; position: absolute; top: 35px; left: 35px; }
        .telemetry { position: absolute; top: 10px; left: 10px; background: rgba(0,0,0,0.7); padding: 10px; border-radius: 5px; font-size: 14px; }
        .telemetry div { margin: 5px 0; }
    </style>
</head>
<body>
    <div class="container">
        <div class="video-container">
            <iframe src="http://localhost:8080/webrtc-streamer/video.html?url=rtsp://localhost:8554/unicast" allowfullscreen></iframe>
            <div class="telemetry">
                <div>🔋 Батарея: <span id="battery">--</span>%</div>
                <div>📐 Крен: <span id="roll">--</span>°</div>
                <div>📐 Тангаж: <span id="pitch">--</span>°</div>
                <div>🎯 Башня: <span id="turret">--</span>°</div>
            </div>
        </div>
        <div class="controls">
            <div class="joystick" id="leftJoystick"><div class="stick"></div></div>
            <div class="joystick" id="rightJoystick"><div class="stick"></div></div>
        </div>
    </div>

    <script>
        let ws;
        const leftStick = document.querySelector('#leftJoystick .stick');
        const rightStick = document.querySelector('#rightJoystick .stick');
        let leftPos = { x: 0, y: 0 };
        let rightPos = { x: 0, y: 0 };

        function connect() {
            ws = new WebSocket('ws://' + window.location.hostname + ':8765');
            ws.onopen = () => console.log('✓ Connected to server');
            ws.onclose = () => setTimeout(connect, 2000);
            ws.onmessage = (event) => {
                const data = JSON.parse(event.data);
                if (data.type === 'TELEMETRY') {
                    document.getElementById('battery').textContent = data.battery;
                    document.getElementById('roll').textContent = data.roll.toFixed(1);
                    document.getElementById('pitch').textContent = data.pitch.toFixed(1);
                    document.getElementById('turret').textContent = data.turret_angle.toFixed(1);
                }
            };
        }

        function sendCommand() {
            const cmd = {
                type: 'COMMAND',
                left: leftPos,
                right: rightPos
            };
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify(cmd));
            }
        }

        function setupJoystick(element, stick, callback) {
            let startX, startY;
            const maxDist = 35;

            element.addEventListener('touchstart', (e) => {
                e.preventDefault();
                const touch = e.touches[0];
                const rect = element.getBoundingClientRect();
                const centerX = rect.left + rect.width / 2;
                const centerY = rect.top + rect.height / 2;
                
                function moveHandler(e) {
                    e.preventDefault();
                    const touch = e.touches[0];
                    let dx = touch.clientX - centerX;
                    let dy = touch.clientY - centerY;
                    const dist = Math.sqrt(dx * dx + dy * dy);
                    
                    if (dist > maxDist) {
                        dx = (dx / dist) * maxDist;
                        dy = (dy / dist) * maxDist;
                    }
                    
                    stick.style.transform = `translate(${dx}px, ${dy}px)`;
                    callback({ x: dx / maxDist, y: dy / maxDist });
                }

                function endHandler(e) {
                    e.preventDefault();
                    stick.style.transform = 'translate(0, 0)';
                    callback({ x: 0, y: 0 });
                    element.removeEventListener('touchmove', moveHandler);
                    element.removeEventListener('touchend', endHandler);
                }

                element.addEventListener('touchmove', moveHandler);
                element.addEventListener('touchend', endHandler);
            });
        }

        setupJoystick(document.getElementById('leftJoystick'), leftStick, (pos) => {
            leftPos = pos;
            sendCommand();
        });

        setupJoystick(document.getElementById('rightJoystick'), rightStick, (pos) => {
            rightPos = pos;
            sendCommand();
        });

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
    
    print("🚀 Запуск Python Bridge...")
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
    
    print(f"✓ Сервер запущен. Откройте в браузере: http://<IP_RPI>:{HTTP_PORT}")
    
    # Бесконечное ожидание
    while True:
        await asyncio.sleep(3600)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n🛑 Остановка Python Bridge...")
        if tcp_socket:
            tcp_socket.close()
