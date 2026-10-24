"""
SMART-GROW — Configuración Central del Servicio Puente de IA
"""

import os

# Configuración del Servidor Puente
BRIDGE_HOST = os.getenv("BRIDGE_HOST", "0.0.0.0")
BRIDGE_PORT = int(os.getenv("BRIDGE_PORT", "8000"))

# Modo de Comunicación con el ESP32: "HTTP" o "SERIAL"
COMMUNICATION_MODE = os.getenv("COMMUNICATION_MODE", "HTTP")
ESP32_HTTP_URL = os.getenv("ESP32_HTTP_URL", "http://192.168.1.150")
ESP32_SERIAL_PORT = os.getenv("ESP32_SERIAL_PORT", "COM3")
ESP32_SERIAL_BAUD = int(os.getenv("ESP32_SERIAL_BAUD", "115200"))

# Configuración de LM Studio (API local compatible con OpenAI)
# Por defecto en LM Studio: http://localhost:1234/v1
LM_STUDIO_BASE_URL = os.getenv("LM_STUDIO_BASE_URL", "http://localhost:1234/v1")
LM_STUDIO_API_KEY = os.getenv("LM_STUDIO_API_KEY", "lm-studio")  # LM Studio no requiere key real
LM_STUDIO_MODEL = os.getenv("LM_STUDIO_MODEL", "local-model")
LM_STUDIO_TIMEOUT_SECONDS = int(os.getenv("LM_STUDIO_TIMEOUT_SECONDS", "30"))

# Modos de Operación de la Inteligencia Artificial
# 1. "OBSERVATION_ONLY": La IA únicamente analiza y explica tendencias sin capacidad de accionar ningún actuador.
# 2. "SUPERVISED_APPROVAL": La IA propone una acción, la capa de seguridad la evalúa, y solo se ejecuta si cumple todas las reglas deterministas.
AI_OPERATING_MODE = os.getenv("AI_OPERATING_MODE", "OBSERVATION_ONLY")

# Límites Deterministas Infranqueables de Seguridad (Safety Guard Hard Limits)
SAFETY_LIMITS = {
    "PUMP_MAX_DURATION_SECONDS": 8,       # Tiempo máximo de bomba por activación
    "PUMP_MIN_COOLDOWN_SECONDS": 180,     # Espera mínima entre riegos (3 minutos)
    "PUMP_MIN_SOIL_THRESHOLD": 35.0,      # No permitir regar si humedad > 35%
    "WINDOW_MAX_ANGLE": 90,               # Límite físico angular
    "WINDOW_MIN_ANGLE": 0,
    "TEMP_FAN_THRESHOLD": 28.0,           # Temperatura mínima para que IA recomiende ventilador
    "AUTO_COOLDOWN_PROTECTION": True
}

# Frecuencia de Sondeo y Telemetría
TELEMETRY_POLL_INTERVAL_SEC = 3
HISTORY_BUFFER_SIZE = 120  # Mantiene las últimas 120 lecturas (~6 minutos)
