"""
SMART-GROW — Punto de Entrada CLI del Servicio Puente de IA
Ejecuta el servicio de puente y expone los endpoints para el Dashboard e integración con LM Studio.
"""

import sys
import argparse
import socketserver
import logging
from config import BRIDGE_HOST, BRIDGE_PORT, ESP32_HTTP_URL, LM_STUDIO_BASE_URL, AI_OPERATING_MODE
from bridge_service import BridgeService, BridgeHTTPHandler

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)
logger = logging.getLogger("BridgeMain")

def main():
    parser = argparse.ArgumentParser(description="SMART-GROW — Servicio Puente ESP32 + IA Local")
    parser.add_argument("--port", type=int, default=BRIDGE_PORT, help=f"Puerto del puente (por defecto: {BRIDGE_PORT})")
    parser.add_argument("--esp32-url", type=str, default=ESP32_HTTP_URL, help="URL HTTP del ESP32")
    parser.add_argument("--lm-url", type=str, default=LM_STUDIO_BASE_URL, help="URL de la API de LM Studio")
    parser.add_argument("--mode", type=str, choices=["OBSERVATION_ONLY", "SUPERVISED_APPROVAL"], 
                        default=AI_OPERATING_MODE, help="Modo de operación de la IA")

    args = parser.parse_args()

    print("================================================================")
    print("  SMART-GROW — Servicio Puente ESP32 + LM Studio (IA Local)")
    print(f"  ESP32 Destino:      {args.esp32_url}")
    print(f"  LM Studio Local:    {args.lm_url}")
    print(f"  Modo Seguridad IA:  {args.mode}")
    print(f"  Servidor Activo en: http://localhost:{args.port}")
    print("================================================================\n")

    service = BridgeService(esp32_url=args.esp32_url, ai_mode=args.mode)
    service.lm_client.base_url = args.lm_url.rstrip("/")
    service.lm_client.endpoint = f"{service.lm_client.base_url}/chat/completions"
    service.start()

    BridgeHTTPHandler.service = service

    class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
        allow_reuse_address = True

    try:
        with ThreadedTCPServer((BRIDGE_HOST, args.port), BridgeHTTPHandler) as server:
            logger.info(f"Escuchando peticiones en {BRIDGE_HOST}:{args.port}...")
            server.serve_forever()
    except KeyboardInterrupt:
        print("\n[INFO] Deteniendo servicio puente...")
    finally:
        service.stop()
        print("[INFO] Servicio puente finalizado con seguridad.")

if __name__ == "__main__":
    main()
