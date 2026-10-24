"""
SMART-GROW — Servidor Web Local para el Dashboard
Sirve los archivos estáticos de la interfaz web y permite conectar con el ESP32 o el puente de IA.
"""

import http.server
import socketserver
import os
import sys
import argparse

DEFAULT_PORT = 8080
FRONTEND_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "frontend"))

class SmartGrowHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FRONTEND_DIR, **kwargs)

    def end_headers(self):
        # Cabeceras CORS y control de caché para desarrollo
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        super().end_headers()

def run_server(port=DEFAULT_PORT):
    if not os.path.isdir(FRONTEND_DIR):
        print(f"[ERROR] No se encontró el directorio frontend en: {FRONTEND_DIR}")
        sys.exit(1)

    handler = SmartGrowHTTPRequestHandler
    with socketserver.TCPServer(("", port), handler) as httpd:
        print("=======================================================")
        print("  SMART-GROW — Servidor Dashboard Local")
        print(f"  Directorio: {FRONTEND_DIR}")
        print(f"  URL Local:  http://localhost:{port}")
        print("=======================================================")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[INFO] Servidor detenido por el usuario.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Servidor local para el Dashboard SMART-GROW")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"Puerto HTTP (por defecto: {DEFAULT_PORT})")
    args = parser.parse_args()
    run_server(args.port)
