"""
SMART-GROW — Emulador de Hardware ESP32-S3 (Mock Server)
Permite validar el Dashboard Web, el Puente de IA y la suite de pruebas
sin necesidad de conectar la placa física de forma inmediata.
"""

import http.server
import socketserver
import json
import time
import threading
import argparse

class MockESP32Hardware:
    def __init__(self):
        # Sensores
        self.temp_base = 23.5
        self.hum_base = 58.0
        self.soil_base = 48.0
        self.light_base = 65.0
        
        # Estados
        self.mode = "AUTO"  # "AUTO" o "MANUAL"
        self.state = "NORMAL_ESTABLE"
        self.sim_drought = False
        self.sim_heat = False
        
        # Actuadores
        self.pump_active = False
        self.pump_started_at = 0.0
        self.pump_duration_s = 8.0
        self.last_pump_stopped_at = 0.0
        self.cooldown_total_s = 180.0
        
        self.fan_active = False
        self.fan_speed_pct = 0
        self.light_duty_pct = 0
        self.window_angle_deg = 0

    def update(self):
        now = time.time()
        # 1. Temporizador de bomba
        if self.pump_active:
            if now - self.pump_started_at >= self.pump_duration_s:
                self.pump_active = False
                self.last_pump_stopped_at = now
                self.soil_base += 15.0  # El riego hidrata el sustrato
                self.soil_base = min(self.soil_base, 85.0)
                print("[MOCK_ESP32] Bomba apagada por timeout de seguridad.")

        # 2. Secado natural del sustrato si no se riega
        if not self.sim_drought:
            self.soil_base -= 0.05
            self.soil_base = max(self.soil_base, 25.0)

        # 3. Lógica automática de control (Histeresis)
        if self.mode == "AUTO":
            eff_soil = 18.0 if self.sim_drought else self.soil_base
            eff_temp = 34.5 if self.sim_heat else self.temp_base

            # Riego
            if eff_soil < 35.0 and not self.pump_active:
                rem_cooldown = self.get_remaining_cooldown()
                if rem_cooldown <= 0:
                    self.pump_active = True
                    self.pump_started_at = now
                    self.state = "REGANDO"
                    print("[MOCK_ESP32 AUTO] Suelo bajo umbral. Iniciando riego.")
            elif eff_soil >= 60.0 and self.pump_active:
                self.pump_active = False
                self.last_pump_stopped_at = now

            # Ventilación
            if eff_temp > 28.0:
                self.fan_active = True
                self.fan_speed_pct = 100
                self.window_angle_deg = 90
                if self.state != "REGANDO":
                    self.state = "VENTILANDO"
            elif eff_temp < 24.5:
                self.fan_active = False
                self.fan_speed_pct = 0
                self.window_angle_deg = 0
                if self.state == "VENTILANDO":
                    self.state = "NORMAL_ESTABLE"

    def get_remaining_cooldown(self) -> int:
        if self.last_pump_stopped_at == 0:
            return 0
        elapsed = time.time() - self.last_pump_stopped_at
        if elapsed < self.cooldown_total_s:
            return int(self.cooldown_total_s - elapsed)
        return 0

    def get_telemetry_dict(self):
        eff_soil = 18.0 if self.sim_drought else round(self.soil_base, 1)
        eff_temp = 34.5 if self.sim_heat else round(self.temp_base, 1)
        eff_hum = 32.0 if self.sim_heat else round(self.hum_base, 1)

        return {
            "device": "LONELY_BINARY_ESP32S3_MOCK",
            "uptime_sec": int(time.time()),
            "mode": self.mode,
            "state": self.state,
            "sensors_real": {
                "temperature_c": round(self.temp_base, 1),
                "humidity_air_pct": round(self.hum_base, 1),
                "soil_moisture_pct": round(self.soil_base, 1),
                "light_level_pct": round(self.light_base, 1),
                "soil_raw_adc": 2400,
                "light_raw_adc": 1800,
                "dht_valid": True,
                "soil_valid": True,
                "has_fault": False,
                "fault_message": "OK"
            },
            "sensors_effective": {
                "temperature_c": eff_temp,
                "humidity_air_pct": eff_hum,
                "soil_moisture_pct": eff_soil,
                "light_level_pct": round(self.light_base, 1)
            },
            "actuators": {
                "pump_active": self.pump_active,
                "pump_cooldown_sec": self.get_remaining_cooldown(),
                "fan_active": self.fan_active,
                "fan_speed_pct": self.fan_speed_pct,
                "light_active": (self.light_duty_pct > 0),
                "light_duty_pct": self.light_duty_pct,
                "window_angle_deg": self.window_angle_deg
            },
            "simulation": {
                "is_active": (self.sim_drought or self.sim_heat),
                "drought_mode": self.sim_drought,
                "heat_mode": self.sim_heat,
                "trigger_source": "MOCK_CONSOLE"
            }
        }

mock_hw = MockESP32Hardware()

class MockESP32Handler(http.server.BaseHTTPRequestHandler):
    def send_cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_cors_headers()
        self.end_headers()

    def do_GET(self):
        mock_hw.update()
        if self.path == "/api/telemetry":
            self.send_response(200)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(mock_hw.get_telemetry_dict()).encode("utf-8"))
        elif self.path == "/api/status":
            self.send_response(200)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"ONLINE","board":"LONELY_BINARY_ESP32S3"}')
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        mock_hw.update()
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length > 0 else "{}"
        try:
            doc = json.loads(body)
        except json.JSONDecodeError:
            doc = {}

        if self.path == "/api/actuators":
            if mock_hw.mode == "AUTO":
                self.send_response(403)
                self.send_cors_headers()
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(b'{"error":"Rechazado: El sistema esta en modo AUTOMATICO"}')
                return

            if doc.get("pump") is True:
                if mock_hw.get_remaining_cooldown() > 0:
                    self.send_response(429)
                    self.send_cors_headers()
                    self.send_header("Content-Type", "application/json")
                    self.end_headers()
                    self.wfile.write(b'{"error":"Bomba en cooldown"}')
                    return
                mock_hw.pump_active = True
                dur_ms = doc.get("duration_ms", 5000)
                mock_hw.pump_duration_s = min(dur_ms / 1000.0, 10.0)
                mock_hw.pump_started_at = time.time()
            elif doc.get("pump") is False:
                mock_hw.pump_active = False
                mock_hw.last_pump_stopped_at = time.time()

            if "fan" in doc:
                mock_hw.fan_active = bool(doc["fan"])
                mock_hw.fan_speed_pct = doc.get("fan_speed", 100) if mock_hw.fan_active else 0

            if "light" in doc:
                mock_hw.light_duty_pct = int(doc["light"])

            if "window" in doc:
                mock_hw.window_angle_deg = int(doc["window"])

            self.send_response(200)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"Comandos ejecutados en Mock"}')

        elif self.path == "/api/mode":
            mode = doc.get("mode", "AUTO").upper()
            if mode in ["AUTO", "MANUAL"]:
                mock_hw.mode = mode
                self.send_response(200)
                self.send_cors_headers()
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(f'{{"status":"Modo {mode} configurado"}}'.encode("utf-8"))
            else:
                self.send_response(400)
                self.end_headers()

        elif self.path == "/api/simulate":
            if "drought" in doc:
                mock_hw.sim_drought = not mock_hw.sim_drought
            if "heat" in doc:
                mock_hw.sim_heat = not mock_hw.sim_heat
            if doc.get("stop"):
                mock_hw.sim_drought = False
                mock_hw.sim_heat = False
            self.send_response(200)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"Simulacion actualizada en Mock"}')

        elif self.path == "/api/emergency":
            mock_hw.pump_active = False
            mock_hw.fan_active = False
            mock_hw.light_duty_pct = 0
            mock_hw.window_angle_deg = 45
            mock_hw.state = "PARADA_EMERGENCIA"
            self.send_response(200)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"EMERGENCY_STOP_EXECUTED"}')
        else:
            self.send_response(404)
            self.end_headers()

def run_mock(port=8181):
    class ThreadedServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
        allow_reuse_address = True

    print("=======================================================")
    print("  SMART-GROW — Emulador Hardware ESP32-S3 (Mock)")
    print(f"  API URL: http://localhost:{port}")
    print("=======================================================")
    with ThreadedServer(("", port), MockESP32Handler) as server:
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print("\n[INFO] Emulador detenido.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8181)
    args = parser.parse_args()
    run_mock(args.port)
