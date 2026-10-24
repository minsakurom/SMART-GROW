"""
SMART-GROW — Pruebas de Integración de la API del Puente y Flujo de Datos
Verifica el ciclo completo entre la telemetría, el buffer de historial y los comandos de actuadores.
"""

import sys
import os
import unittest
import time
import threading

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "ai_bridge")))
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))

from bridge_service import BridgeService
from mock_esp32_server import MockESP32Hardware

class TestBridgeIntegration(unittest.TestCase):
    def setUp(self):
        self.mock_hw = MockESP32Hardware()
        self.bridge = BridgeService(esp32_url="http://127.0.0.1:8181", ai_mode="SUPERVISED_APPROVAL")

    def test_telemetry_processing_and_summary(self):
        """El puente debe ingerir correctamente la telemetría del ESP32 y calcular resúmenes."""
        raw_telemetry = self.mock_hw.get_telemetry_dict()
        self.bridge._process_incoming_telemetry(raw_telemetry)

        self.assertEqual(len(self.bridge.history), 1)
        summary = self.bridge.get_summary()

        self.assertEqual(summary["temperature"], 23.5)
        self.assertEqual(summary["soil_moisture"], 48.0)
        self.assertEqual(summary["mode"], "AUTO")
        self.assertFalse(summary["is_simulated"])

    def test_simulation_effective_values_extraction(self):
        """Cuando la simulación está activa, el resumen debe reflejar los valores forzados."""
        self.mock_hw.sim_drought = True
        sim_telemetry = self.mock_hw.get_telemetry_dict()
        self.bridge._process_incoming_telemetry(sim_telemetry)

        summary = self.bridge.get_summary()
        self.assertEqual(summary["soil_moisture"], 18.0)
        self.assertTrue(summary["is_simulated"])

    def test_safety_guard_blocks_unauthorized_pump_during_cooldown(self):
        """Verifica que el servicio puente proteja la bomba mediante cooldown."""
        telemetry = self.mock_hw.get_telemetry_dict()
        self.bridge._process_incoming_telemetry(telemetry)

        # 1. Simular que la bomba terminó de regar
        self.bridge.safety_guard.notify_pump_stopped()

        # 2. Intentar solicitar riego
        approved, reason, sanitized = self.bridge.safety_guard.validate_action(
            {"pump": True, "duration_seconds": 5},
            self.bridge.current_telemetry,
            origin="WEB_DASHBOARD"
        )

        self.assertFalse(approved)
        self.assertIn("Cooldown activo", reason)
        self.assertIsNone(sanitized)

if __name__ == "__main__":
    unittest.main()
