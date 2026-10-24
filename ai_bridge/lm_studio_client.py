"""
SMART-GROW — Cliente de Interacción con LM Studio (API OpenAI Compatible)
Se comunica con el servidor local de LM Studio para obtener análisis agronómicos y diagnósticos.
"""

import json
import urllib.request
import urllib.error
import logging
import re
from typing import Dict, Any, Optional
from config import LM_STUDIO_BASE_URL, LM_STUDIO_MODEL, LM_STUDIO_TIMEOUT_SECONDS

logger = logging.getLogger("LMStudioClient")

SYSTEM_PROMPT = """Eres el Especialista Agronómico y de Automatización de SMART-GROW, un invernadero escolar automatizado.
Tu labor es analizar las condiciones del invernadero (temperatura, humedad de aire, humedad de suelo capacitiva, luz y estados de actuadores), explicar las tendencias de forma clara para estudiantes y proponer ajustes si es estrictamente necesario.

REGLAS DE SEGURIDAD ESTRICTAS:
1. No inventes datos que no figuren en la telemetría.
2. Si los valores son normales (Suelo 40-60%, Temp 20-26°C), NO propongas regar ni ventilar.
3. Si el suelo está seco (<35%), puedes proponer regar indicando 'pump': true, pero NUNCA solicites más de 8 segundos de riego.
4. Si la temperatura supera 28°C, puedes proponer ventilación ('fan': true, 'window_angle': 90).
5. Debes responder SIEMPRE en formato JSON estricto con la siguiente estructura:
{
  "analysis": "Explicación clara del estado actual y tendencias para los estudiantes.",
  "action_needed": false,
  "proposed_action": null,
  "educational_tip": "Dato científico o botánico formativo sobre el cultivo."
}
Si se requiere acción, 'proposed_action' puede contener:
{ "pump": true, "duration_seconds": 5, "fan": false, "window_angle": 0, "light_pct": 80 }
"""

class LMStudioClient:
    def __init__(self, base_url: str = LM_STUDIO_BASE_URL, model: str = LM_STUDIO_MODEL):
        self.base_url = base_url.rstrip("/")
        self.endpoint = f"{self.base_url}/chat/completions"
        self.model = model

    def check_connection(self) -> bool:
        """Comprueba si el servidor local de LM Studio está respondiendo."""
        try:
            models_url = f"{self.base_url}/models"
            req = urllib.request.Request(models_url, headers={"User-Agent": "SMART-GROW-Bridge"})
            with urllib.request.urlopen(req, timeout=3) as resp:
                return resp.status == 200
        except Exception:
            return False

    def request_analysis(self, greenhouse_summary: Dict[str, Any]) -> Dict[str, Any]:
        """
        Envía un resumen de telemetría a LM Studio y obtiene un diagnóstico estructurado.
        """
        user_message = f"""Telemetría actual del invernadero SMART-GROW:
- Temperatura interior: {greenhouse_summary.get('temperature', '--')} °C
- Humedad ambiental: {greenhouse_summary.get('air_humidity', '--')} % RH
- Humedad de suelo: {greenhouse_summary.get('soil_moisture', '--')} % Relativo
- Iluminación: {greenhouse_summary.get('light_level', '--')} %
- Estado Bomba: {'ENCENDIDA' if greenhouse_summary.get('pump_active') else 'APAGADA'}
- Estado Ventilador: {'ENCENDIDO' if greenhouse_summary.get('fan_active') else 'APAGADO'}
- Estado Ventana: {greenhouse_summary.get('window_angle', 0)} grados
- Modo de operación: {greenhouse_summary.get('mode', 'AUTO')}
- Modo Simulación: {'ACTIVO' if greenhouse_summary.get('is_simulated') else 'DESACTIVADO'}

Analiza el estado y genera el diagnóstico en el formato JSON solicitado."""

        payload = {
            "model": self.model,
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": user_message}
            ],
            "temperature": 0.3,  # Baja temperatura para respuestas consistentes y deterministas
            "max_tokens": 500
        }

        req_data = json.dumps(payload).encode("utf-8")
        headers = {
            "Content-Type": "application/json",
            "User-Agent": "SMART-GROW-Bridge"
        }

        req = urllib.request.Request(self.endpoint, data=req_data, headers=headers, method="POST")

        try:
            logger.info(f"Consultando LM Studio en {self.endpoint}...")
            with urllib.request.urlopen(req, timeout=LM_STUDIO_TIMEOUT_SECONDS) as response:
                res_body = response.read().decode("utf-8")
                res_json = json.loads(res_body)
                content = res_json["choices"][0]["message"]["content"]
                return self._parse_json_response(content)

        except urllib.error.URLError as e:
            logger.warning(f"No se pudo conectar con LM Studio en {self.endpoint}: {e}")
            return {
                "analysis": "El servidor local de LM Studio no se encuentra activo en el puerto 1234. Inicie el Local Server en LM Studio.",
                "action_needed": False,
                "proposed_action": None,
                "educational_tip": "La IA local opera de forma desconectada de Internet, resguardando la privacidad de la escuela.",
                "error": str(e)
            }
        except Exception as e:
            logger.error(f"Error inesperado al procesar respuesta de LM Studio: {e}")
            return {
                "analysis": f"Falla en la inferencia del modelo local: {str(e)}",
                "action_needed": False,
                "proposed_action": None,
                "educational_tip": "Verifique que el modelo cargado soporte generación de instrucciones en español.",
                "error": str(e)
            }

    def _parse_json_response(self, text: str) -> Dict[str, Any]:
        """Extrae de forma robusta el JSON del bloque de texto o markdown retornado."""
        # Intento de extracción directa
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            pass

        # Buscar bloques ```json ... ```
        match = re.search(r"```(?:json)?\s*(\{.*?\})\s*```", text, re.DOTALL)
        if match:
            try:
                return json.loads(match.group(1))
            except json.JSONDecodeError:
                pass

        # Si el modelo no generó JSON válido, se encapsula su respuesta como análisis
        return {
            "analysis": text.strip(),
            "action_needed": False,
            "proposed_action": None,
            "educational_tip": "La fotosíntesis óptima requiere equilibrio entre luz, agua y temperatura estable."
        }
