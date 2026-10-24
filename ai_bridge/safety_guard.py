"""
SMART-GROW — Capa Determinista de Seguridad (Safety Guard)
Garantiza que ninguna recomendación o comando generado por la IA o el usuario
pueda violar los límites físicos del hardware o poner en riesgo las plantas.
"""

import time
import logging
from typing import Dict, Any, Tuple, Optional
from config import SAFETY_LIMITS, AI_OPERATING_MODE

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger("SafetyGuard")

class DeterministicSafetyGuard:
    def __init__(self, mode: str = AI_OPERATING_MODE):
        self.operating_mode = mode
        self.last_pump_stopped_at = 0.0
        self.audit_log = []

    def set_mode(self, mode: str):
        if mode in ["OBSERVATION_ONLY", "SUPERVISED_APPROVAL"]:
            self.operating_mode = mode
            logger.info(f"Modo de seguridad actualizado a: {mode}")
        else:
            raise ValueError(f"Modo inválido: {mode}")

    def notify_pump_stopped(self):
        """Registra el instante en que la bomba cesó de operar para el cálculo del cooldown."""
        self.last_pump_stopped_at = time.time()

    def get_remaining_cooldown(self) -> float:
        elapsed = time.time() - self.last_pump_stopped_at
        required = SAFETY_LIMITS["PUMP_MIN_COOLDOWN_SECONDS"]
        if elapsed < required:
            return required - elapsed
        return 0.0

    def validate_action(self, 
                        proposed_action: Optional[Dict[str, Any]], 
                        current_telemetry: Dict[str, Any],
                        origin: str = "AI_MODEL") -> Tuple[bool, str, Optional[Dict[str, Any]]]:
        """
        Evalúa una acción propuesta contra las reglas deterministas de seguridad.
        Retorna: (is_approved: bool, reason: str, sanitized_command: Optional[Dict])
        """
        # Regla 0: Si no hay acción propuesta o es nula
        if not proposed_action:
            return False, "No se propuso ninguna acción concreta de actuador.", None

        # Regla 1: Modo Observador Rígido
        if self.operating_mode == "OBSERVATION_ONLY" and origin == "AI_MODEL":
            msg = "RECHAZADO: El sistema opera en MODO OBSERVADOR estricto. La IA no tiene autorización para accionar hardware."
            self._log_audit(origin, proposed_action, False, msg)
            return False, msg, None

        sanitized = {}

        # Regla 2: Inspección y saneamiento de activación de Bomba de Agua
        if "pump" in proposed_action and proposed_action["pump"] is True:
            # 2.1 Verificación de cooldown
            remaining_cooldown = self.get_remaining_cooldown()
            if remaining_cooldown > 0:
                msg = f"RECHAZADO: Cooldown activo en sustrato. Restan {remaining_cooldown:.1f} s para permitir nuevo riego."
                self._log_audit(origin, proposed_action, False, msg)
                return False, msg, None

            # 2.2 Verificación de necesidad hídrica real (no regar si el suelo ya está húmedo)
            sensors = current_telemetry.get("sensors_effective", current_telemetry.get("sensors_real", {}))
            soil_pct = float(sensors.get("soil_moisture_pct", 50.0))
            if soil_pct >= SAFETY_LIMITS["PUMP_MIN_SOIL_THRESHOLD"] and origin == "AI_MODEL":
                msg = f"RECHAZADO: La humedad actual ({soil_pct:.1f}%) supera el umbral crítico ({SAFETY_LIMITS['PUMP_MIN_SOIL_THRESHOLD']}%). Riego innecesario."
                self._log_audit(origin, proposed_action, False, msg)
                return False, msg, None

            # 2.3 Clampeo estricto del tiempo máximo de funcionamiento
            requested_duration = proposed_action.get("duration_seconds", SAFETY_LIMITS["PUMP_MAX_DURATION_SECONDS"])
            try:
                requested_duration = float(requested_duration)
            except (ValueError, TypeError):
                requested_duration = SAFETY_LIMITS["PUMP_MAX_DURATION_SECONDS"]

            clamped_duration = min(max(requested_duration, 1.0), SAFETY_LIMITS["PUMP_MAX_DURATION_SECONDS"])
            sanitized["pump"] = True
            sanitized["duration_ms"] = int(clamped_duration * 1000)

        elif "pump" in proposed_action and proposed_action["pump"] is False:
            sanitized["pump"] = False

        # Regla 3: Inspección de Ventilador
        if "fan" in proposed_action:
            fan_val = bool(proposed_action["fan"])
            speed = proposed_action.get("fan_speed", 100)
            try:
                speed = int(speed)
                speed = min(max(speed, 0), 100)
            except (ValueError, TypeError):
                speed = 100
            sanitized["fan"] = fan_val
            sanitized["fan_speed"] = speed

        # Regla 4: Inspección de Ventana Servomotor
        if "window_angle" in proposed_action:
            try:
                ang = int(proposed_action["window_angle"])
                ang = min(max(ang, SAFETY_LIMITS["WINDOW_MIN_ANGLE"]), SAFETY_LIMITS["WINDOW_MAX_ANGLE"])
                sanitized["window"] = ang
            except (ValueError, TypeError):
                pass

        # Regla 5: Inspección de Iluminación
        if "light_pct" in proposed_action:
            try:
                lgt = int(proposed_action["light_pct"])
                lgt = min(max(lgt, 0), 100)
                sanitized["light"] = lgt
            except (ValueError, TypeError):
                pass

        if not sanitized:
            msg = "RECHAZADO: La propuesta de la IA contenía parámetros desconocidos o no válidos."
            self._log_audit(origin, proposed_action, False, msg)
            return False, msg, None

        msg = "APROBADO: Acción verificada y saneada bajo los límites deterministas de seguridad."
        self._log_audit(origin, sanitized, True, msg)
        return True, msg, sanitized

    def _log_audit(self, origin: str, action: Any, approved: bool, reason: str):
        record = {
            "timestamp": time.time(),
            "origin": origin,
            "action": action,
            "approved": approved,
            "reason": reason
        }
        self.audit_log.append(record)
        if len(self.audit_log) > 100:
            self.audit_log.pop(0)
        logger.info(f"[AUDIT] {origin} -> Aprobado={approved} | {reason}")
