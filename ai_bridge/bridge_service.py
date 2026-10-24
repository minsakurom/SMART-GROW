"""
SMART-GROW — Servicio Puente Principal (Bridge Service)
Coordina la adquisición de datos del ESP32, el buffer de historial,
el análisis con LM Studio y la validación determinista de seguridad.
"""

from ast import Tuple
import json
import time
import threading
import urllib.request
import urllib.error
import http.server
import socketserver
import logging
from typing import Dict, Any, List, Optional

from config import (
    BRIDGE_HOST, BRIDGE_PORT, ESP32_HTTP_URL, 
    TELEMETRY_POLL_INTERVAL_SEC, HISTORY_BUFFER_SIZE, AI_OPERATING_MODE
)
from safety_guard import DeterministicSafetyGuard
from lm_studio_client import LMStudioClient

logger = logging.getLogger("BridgeService")

class BridgeService:
    def __init__(self, esp32_url: str = ESP32_HTTP_URL, ai_mode: str = AI_OPERATING_MODE):
        self.esp32_url = esp32_url.rstrip("/")
        self.safety_guard = DeterministicSafetyGuard(mode=ai_mode)
        self.lm_client = LMStudioClient()
        
        self.history: List[Dict[str, Any]] = []
        self.current_telemetry: Dict[str, Any] = {}
        self.last_ai_result: Dict[str, Any] = {
            "analysis": "Sistema puente iniciado. En espera de primera consulta a LM Studio.",
            "action_needed": False,
            "proposed_action": None,
            "approved_action": None,
            "timestamp": time.time()
        }
        
        self._is_running = False
        self._poller_thread: Optional[threading.Thread] = None

    def start(self):
        self._is_running = True
        self._poller_thread = threading.Thread(target=self._telemetry_poller_loop, daemon=True)
        self._poller_thread.start()
        logger.info(f"Servicio puente iniciado. Conectando con ESP32 en {self.esp32_url}...")

    def stop(self):
        self._is_running = False
        if self._poller_thread and self._poller_thread.is_alive():
            self._poller_thread.join(timeout=2.0)
        logger.info("Servicio puente detenido.")

    def _telemetry_poller_loop(self):
        """Bucle en segundo plano para obtener telemetría periódica del ESP32."""
        while self._is_running:
            try:
                endpoint = f"{self.esp32_url}/api/telemetry"
                req = urllib.request.Request(endpoint, headers={"User-Agent": "SMART-GROW-Bridge"})
                with urllib.request.urlopen(req, timeout=2.5) as resp:
                    if resp.status == 200:
                        data = json.loads(resp.read().decode("utf-8"))
                        self._process_incoming_telemetry(data)
            except Exception as e:
                logger.debug(f"Sondeo ESP32 no disponible en {self.esp32_url}: {e}")
            
            time.sleep(TELEMETRY_POLL_INTERVAL_SEC)

    def _process_incoming_telemetry(self, telemetry: Dict[str, Any]):
        """Valida e incorpora la telemetría al buffer histórico."""
        telemetry["_received_at"] = time.time()
        self.current_telemetry = telemetry

        # Si la bomba dejó de regar, avisar al guardián de seguridad para el cooldown
        actuators = telemetry.get("actuators", {})
        if not actuators.get("pump_active", False):
            self.safety_guard.notify_pump_stopped()

        self.history.append(telemetry)
        if len(self.history) > HISTORY_BUFFER_SIZE:
            self.history.pop(0)

    def get_summary(self) -> Dict[str, Any]:
        """Genera un resumen analítico con tendencias calculadas para la IA."""
        if not self.current_telemetry:
            return {
                "temperature": 23.5,
                "air_humidity": 55.0,
                "soil_moisture": 45.0,
                "light_level": 60.0,
                "pump_active": False,
                "fan_active": False,
                "window_angle": 0,
                "mode": "AUTO",
                "is_simulated": False,
                "status": "MOCK_OR_INITIALIZING"
            }

        s_eff = self.current_telemetry.get("sensors_effective", 
                 self.current_telemetry.get("sensors_real", {}))
        act = self.current_telemetry.get("actuators", {})
        sim = self.current_telemetry.get("simulation", {})

        return {
            "temperature": float(s_eff.get("temperature_c", 22.0)),
            "air_humidity": float(s_eff.get("humidity_air_pct", 50.0)),
            "soil_moisture": float(s_eff.get("soil_moisture_pct", 45.0)),
            "light_level": float(s_eff.get("light_level_pct", 50.0)),
            "pump_active": bool(act.get("pump_active", False)),
            "fan_active": bool(act.get("fan_active", False)),
            "window_angle": int(act.get("window_angle_deg", 0)),
            "mode": self.current_telemetry.get("mode", "AUTO"),
            "is_simulated": bool(sim.get("is_active", False)),
            "status": "ONLINE"
        }

    def run_ai_consultation(self) -> Dict[str, Any]:
        """Consulta a LM Studio y valida la respuesta deterministamente."""
        summary = self.get_summary()
        raw_result = self.lm_client.request_analysis(summary)

        proposed_action = raw_result.get("proposed_action")
        is_approved = False
        approval_reason = "No aplica"
        sanitized_command = None

        if proposed_action:
            is_approved, approval_reason, sanitized_command = self.safety_guard.validate_action(
                proposed_action=proposed_action,
                current_telemetry=self.current_telemetry,
                origin="AI_MODEL"
            )

        self.last_ai_result = {
            "analysis": raw_result.get("analysis", "Sin análisis."),
            "educational_tip": raw_result.get("educational_tip", ""),
            "action_proposed": proposed_action,
            "action_approved": is_approved,
            "approval_reason": approval_reason,
            "sanitized_command": sanitized_command,
            "timestamp": time.time()
        }

        # En modo supervisado, si fue aprobada una acción, se puede despachar al ESP32
        if is_approved and sanitized_command and self.safety_guard.operating_mode == "SUPERVISED_APPROVAL":
            self.forward_actuator_command(sanitized_command)

        return self.last_ai_result

    def forward_actuator_command(self, command: Dict[str, Any]) -> Tuple[bool, str]:
        """Envía un comando saneado hacia el ESP32."""
        try:
            url = f"{self.esp32_url}/api/actuators"
            data = json.dumps(command).encode("utf-8")
            req = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"}, method="POST")
            with urllib.request.urlopen(req, timeout=3.0) as resp:
                res_txt = resp.read().decode("utf-8")
                return True, res_txt
        except Exception as e:
            logger.error(f"Error al enviar actuadores al ESP32: {e}")
            return False, str(e)


# Servidor HTTP Integrado para el Dashboard y API Local
class BridgeHTTPHandler(http.server.BaseHTTPRequestHandler):
    service: Optional[BridgeService] = None

    def send_cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_cors_headers()
        self.end_headers()

    def do_GET(self):
        if self.path == "/api/telemetry":
            self.send_response(200)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            
            data = self.service.current_telemetry
            if not data:
                # Si no hay ESP32 físico aún conectado, emitimos datos de diagnóstico
                summary = self.service.get_summary()
                data = {
                    "device": "SMART-GROW-BRIDGE-SIM",
                    "mode": summary["mode"],
                    "sensors_real": {
                        "temperature_c": summary["temperature"],
                        "humidity_air_pct": summary["air_humidity"],
                        "soil_moisture_pct": summary["soil_moisture"],
                        "light_level_pct": summary["light_level"]
                    },
                    "sensors_effective": {
                        "temperature_c": summary["temperature"],
                        "humidity_air_pct": summary["air_humidity"],
                        "soil_moisture_pct": summary["soil_moisture"],
                        "light_level_pct": summary["light_level"]
                    },
                    "actuators": {
                        "pump_active": summary["pump_active"],
                        "pump_cooldown_sec": int(self.service.safety_guard.get_remaining_cooldown()),
                        "fan_active": summary["fan_active"],
                        "window_angle_deg": summary["window_angle"]
                    },
                    "simulation": {
                        "is_active": summary["is_simulated"],
                        "drought_mode": False,
                        "heat_mode": False
                    }
                }
            self.wfile.write(json.dumps(data).encode("utf-8"))

        elif self.path == "/api/ai/status":
            self.send_response(200)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            status_data = {
                "ai_mode": self.service.safety_guard.operating_mode,
                "lm_studio_connected": self.service.lm_client.check_connection(),
                "last_result": self.service.last_ai_result
            }
            self.wfile.write(json.dumps(status_data).encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        content_len = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_len).decode("utf-8") if content_len > 0 else "{}"
        
        try:
            req_json = json.loads(body) if body else {}
        except json.JSONDecodeError:
            req_json = {}

        if self.path == "/api/ai/analyze":
            result = self.service.run_ai_consultation()
            self.send_response(200)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(result).encode("utf-8"))

        elif self.path == "/api/actuators":
            # Pasa primero por la capa de seguridad determinista
            approved, reason, sanitized = self.service.safety_guard.validate_action(
                proposed_action=req_json,
                current_telemetry=self.service.current_telemetry,
                origin="WEB_DASHBOARD"
            )
            if not approved:
                self.send_response(403)
                self.send_cors_headers()
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"error": reason}).encode("utf-8"))
                return

            ok, msg = self.service.forward_actuator_command(sanitized)
            code = 200 if ok else 502
            self.send_response(code)
            self.send_cors_headers()
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": msg, "sanitized": sanitized}).encode("utf-8"))

        elif self.path in ["/api/mode", "/api/simulate", "/api/emergency"]:
            # Reenvío directo al ESP32
            target_url = f"{self.service.esp32_url}{self.path}"
            try:
                data = body.encode("utf-8")
                req = urllib.request.Request(target_url, data=data, headers={"Content-Type": "application/json"}, method="POST")
                with urllib.request.urlopen(req, timeout=3.0) as resp:
                    resp_data = resp.read()
                    self.send_response(resp.status)
                    self.send_cors_headers()
                    self.send_header("Content-Type", "application/json")
                    self.end_headers()
                    self.wfile.write(resp_data)
            except Exception as e:
                self.send_response(502)
                self.send_cors_headers()
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"error": f"Falla de reenvio al ESP32: {e}"}).encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()
