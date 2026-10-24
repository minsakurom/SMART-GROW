"""
SMART-GROW — Pruebas Automatizadas de la Capa de Seguridad (Safety Guard)
Valida rigurosamente que ninguna propuesta de la IA o de comandos externos pueda violar
los límites de protección física del invernadero.
"""

import sys
import os
import unittest
import time

# Incorporar directorio de ai_bridge al PATH de Python
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "ai_bridge")))

from safety_guard import DeterministicSafetyGuard
from config import SAFETY_LIMITS

class TestDeterministicSafetyGuard(unittest.TestCase):
    def setUp(self):
        self.guard = DeterministicSafetyGuard(mode="SUPERVISED_APPROVAL")
        self.mock_telemetry_dry = {
            "sensors_real": {"soil_moisture_pct": 20.0, "temperature_c": 24.0},
            "sensors_effective": {"soil_moisture_pct": 20.0, "temperature_c": 24.0},
            "actuators": {"pump_active": False}
        }
        self.mock_telemetry_wet = {
            "sensors_real": {"soil_moisture_pct": 65.0, "temperature_c": 24.0},
            "sensors_effective": {"soil_moisture_pct": 65.0, "temperature_c": 24.0},
            "actuators": {"pump_active": False}
        }

    def test_observation_only_mode_blocks_all_ai_actuations(self):
        """En modo OBSERVATION_ONLY, cualquier orden de actuador sugerida por IA debe ser denegada."""
        self.guard.set_mode("OBSERVATION_ONLY")
        action = {"pump": True, "duration_seconds": 5}
        approved, reason, sanitized = self.guard.validate_action(action, self.mock_telemetry_dry, origin="AI_MODEL")
        
        self.assertFalse(approved)
        self.assertIn("MODO OBSERVADOR", reason)
        self.assertIsNone(sanitized)

    def test_pump_excessive_duration_clamping(self):
        """Si la IA solicita regar por 999 segundos, la seguridad debe clampear al límite estricto (8s)."""
        action = {"pump": True, "duration_seconds": 999}
        approved, reason, sanitized = self.guard.validate_action(action, self.mock_telemetry_dry, origin="AI_MODEL")

        self.assertTrue(approved)
        self.assertEqual(sanitized["pump"], True)
        self.assertEqual(sanitized["duration_ms"], SAFETY_LIMITS["PUMP_MAX_DURATION_SECONDS"] * 1000)

    def test_pump_cooldown_enforcement(self):
        """La bomba no debe poder activarse de nuevo antes de cumplir el período de enfriamiento."""
        # 1. Primer riego
        action = {"pump": True, "duration_seconds": 4}
        approved1, _, sanitized1 = self.guard.validate_action(action, self.mock_telemetry_dry, origin="AI_MODEL")
        self.assertTrue(approved1)

        # 2. Se apaga la bomba
        self.guard.notify_pump_stopped()

        # 3. Intento inmediato de segundo riego (cooldown activo)
        approved2, reason2, sanitized2 = self.guard.validate_action(action, self.mock_telemetry_dry, origin="AI_MODEL")
        self.assertFalse(approved2)
        self.assertIn("Cooldown activo", reason2)
        self.assertIsNone(sanitized2)

    def test_reject_watering_when_soil_is_already_wet(self):
        """Si la IA propone regar pero el sustrato ya tiene humedad suficiente, debe rechazarse."""
        action = {"pump": True, "duration_seconds": 5}
        approved, reason, sanitized = self.guard.validate_action(action, self.mock_telemetry_wet, origin="AI_MODEL")

        self.assertFalse(approved)
        self.assertIn("supera el umbral", reason)
        self.assertIsNone(sanitized)

    def test_window_servo_bounds_clamping(self):
        """El ángulo del servomotor debe quedar estrictamente restringido entre 0° y 90°."""
        # Intento de ángulo negativo
        action_neg = {"window_angle": -45}
        approved_neg, _, sanitized_neg = self.guard.validate_action(action_neg, self.mock_telemetry_dry, origin="AI_MODEL")
        self.assertTrue(approved_neg)
        self.assertEqual(sanitized_neg["window"], 0)

        # Intento de ángulo excesivo
        action_over = {"window_angle": 180}
        approved_over, _, sanitized_over = self.guard.validate_action(action_over, self.mock_telemetry_dry, origin="AI_MODEL")
        self.assertTrue(approved_over)
        self.assertEqual(sanitized_over["window"], 90)

    def test_rejection_of_empty_or_malformed_actions(self):
        """Acciones vacías o sin parámetros válidos deben ser rechazadas limpiamente."""
        approved, reason, sanitized = self.guard.validate_action(None, self.mock_telemetry_dry)
        self.assertFalse(approved)
        self.assertIsNone(sanitized)

        approved2, reason2, sanitized2 = self.guard.validate_action({}, self.mock_telemetry_dry)
        self.assertFalse(approved2)
        self.assertIsNone(sanitized2)

if __name__ == "__main__":
    unittest.main()
