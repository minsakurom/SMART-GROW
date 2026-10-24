# SMART-GROW — Invernadero Inteligente Escolar con ESP32-S3 e IA Local

Proyecto integral de automatización agrícola para ferias científicas y educación tecnológica, diseñado bajo estrictos estándares de ingeniería electrónica, firmware determinista en C++, dashboard web local y diagnóstico agronómico mediante modelos de lenguaje locales (LM Studio).

---

## 1. Arquitectura General del Sistema

```
                             +-------------------------------+
                             |    FUENTE EXTERNA 5V / 3A     |
                             +-------------------------------+
                                 | (VCC 5V)       | (GND)
                                 v                v
+-------------------+      +-----------------------------------------+
| SENSORES (3.3V)   |      |  CONTROLADOR PRINCIPAL:                 |
| - Suelo (GPIO1)   |----->|  LONELY BINARY ESP32-S3 N16R8           |
| - LDR (GPIO2)     |----->|  - 16 MB Flash OPI / 8 MB PSRAM OPI     |
| - DHT22 (GPIO4)   |<---->|  - Lógica 3.3V / Dual USB-C             |
| - OLED (GPIO8,9)  |<---->|  - Máquina de Estados con Histéresis    |
| - Pulsadores(5,6) |----->|  - Servidor REST API Integrado          |
+-------------------+      +-----------------------------------------+
                                 | (PWM / Control 3.3V)
                                 v
                           +-----------------------------------------+
                           | ETAPAS MOSFET & ACTUADORES POTENCIA     |
                           | - Bomba Riego (GPIO14 + 1N5819 Flyback) |
                           | - Ventilador DC (GPIO15 + Flyback)      |
                           | - Iluminación Fotosintética (GPIO16)    |
                           | - Servomotor Ventana SG90 (GPIO17)      |
                           +-----------------------------------------+
                                 ^
                                 | Wi-Fi Local (REST) / USB Serial
                                 v
+-------------------+      +-----------------------------------------+      +--------------------+
| DASHBOARD WEB     |<---->| PUENTE PYTHON (ai_bridge)               |<---->| LM STUDIO LOCAL    |
| - Monitoreo Vivo  | HTTP | - Capa Determinista (Safety Guard)      | API  | - IA Local         |
| - Gráficas Tiempo | REST | - Buffer Histórico y Estadísticas       | HTTP | - Puerto 1234      |
| - Modo Simulación |      | - Modo Observador / Supervisado         |      | - Offline          |
+-------------------+      +-----------------------------------------+      +--------------------+
```

---

## 2. Estructura del Repositorio

```
SMART-GROW/
├── firmware/                     # Código fuente Arduino / C++ para ESP32-S3
│   ├── config/
│   │   └── GreenhouseConfig.h    # Configuración de umbrales, pines y tiempos
│   ├── include/                  # Cabeceras de subsistemas modulares
│   │   ├── SensorsManager.h
│   │   ├── ActuatorsManager.h
│   │   ├── SimulationManager.h
│   │   ├── StateMachine.h
│   │   ├── DisplayManager.h
│   │   └── NetworkManager.h
│   ├── src/                      # Implementación C++ no bloqueante
│   │   ├── SensorsManager.cpp
│   │   ├── ActuatorsManager.cpp
│   │   ├── SimulationManager.cpp
│   │   ├── StateMachine.cpp
│   │   ├── DisplayManager.cpp
│   │   ├── NetworkManager.cpp
│   │   └── main.cpp              # Punto de entrada y loop principal
│   ├── platformio.ini            # Configuración para PlatformIO (16MB Flash, 8MB PSRAM)
│   └── README.md                 # Guía de compilación y carga
│
├── hardware/                     # Documentación y esquemas eléctricos
│   ├── pinout.md                 # Pinout verificado de la placa Lonely Binary
│   ├── wiring.md                 # Tabla de conexiones pin a pin y masas
│   ├── bom.csv                   # Lista de materiales y componentes exactos
│   └── diagrams/
│       ├── mosfet_schematic.md   # Esquema de etapas MOSFET y diodos flyback
│       └── system_architecture.mermaid # Diagrama completo en Mermaid
│
├── dashboard/                    # Interfaz web moderna para supervisión y control
│   ├── frontend/
│   │   ├── index.html            # Interfaz responsiva y accesible
│   │   ├── css/style.css         # Diseño visual oscuro con glassmorphism
│   │   └── js/app.js             # Controlador con Chart.js y enlace HTTP
│   └── backend/
│       └── server.py             # Servidor HTTP local liviano
│
├── ai_bridge/                    # Servicio puente con LM Studio y Capa de Seguridad
│   ├── config.py                 # Parámetros de red y límites deterministas
│   ├── safety_guard.py           # Guardián determinista de seguridad física
│   ├── lm_studio_client.py       # Cliente local OpenAI-compatible para LM Studio
│   ├── bridge_service.py         # Orquestador y servidor de telemetría
│   ├── main.py                   # Ejecutable CLI del puente
│   └── README.md                 # Instrucciones de configuración de LM Studio
│
├── docs/
│   └── calibration_guide.md      # Protocolo de calibración de 2 puntos para sensores
│
├── tests/                        # Suite de pruebas unitarias y de integración
│   ├── test_safety_guard.py      # Pruebas de límites de la bomba y cooldown
│   ├── test_bridge_api.py        # Pruebas de ingestión de telemetría
│   ├── mock_esp32_server.py      # Emulador de hardware para pruebas sin placa
│   └── test_plan.md              # Plan de pruebas progresivo de 6 niveles
│
└── README.md                     # Documentación central del proyecto
```

---

## 3. Guía de Inicio Rápido (Quickstart)

### Opción A: Prueba Inmediata en el Ordenador (Sin Hardware Físico)
Puedes validar el funcionamiento completo del sistema ahora mismo ejecutando el emulador de hardware, el puente de IA y el dashboard web:

1. **Terminal 1 — Iniciar el Emulador del ESP32-S3**:
   ```bash
   cd SMART-GROW/tests
   python mock_esp32_server.py --port 8181
   ```
2. **Terminal 2 — Iniciar el Puente de Inteligencia Artificial**:
   ```bash
   cd SMART-GROW/ai_bridge
   python main.py --port 8000 --esp32-url http://127.0.0.1:8181 --mode OBSERVATION_ONLY
   ```
3. **Terminal 3 — Iniciar el Dashboard Web**:
   ```bash
   cd SMART-GROW/dashboard/backend
   python server.py --port 8080
   ```
4. Abrir en el navegador: **`http://localhost:8080`**.
   - En la caja superior de IP del dispositivo, colocar: `127.0.0.1:8000`.
   - Podrás observar la telemetría en tiempo real, probar los botones de simulación escolar (Sequía / Calor), interactuar con las gráficas y verificar los interlocks de seguridad.

---

### Opción B: Carga en Hardware Real (ESP32-S3)
1. Conectar la placa **LONELY BINARY ESP32-S3 N16R8 Gold Edition** por cable USB.
2. Compilar y subir el firmware utilizando **PlatformIO**:
   ```bash
   cd SMART-GROW/firmware
   pio run --target upload
   ```
   *(O utilizando Arduino IDE según las instrucciones detalladas en `firmware/README.md`).*
3. Abrir el monitor serial a 115200 baudios para leer la dirección IP asignada por la red Wi-Fi local.
4. Conectar los sensores y actuadores respetando estrictamente el diagrama de `hardware/wiring.md`.

---

## 4. Guía de Resolución de Problemas (Troubleshooting)

| Síntoma | Causa Posible | Solución Verificada |
|---|---|---|
| **La pantalla OLED SSD1306 no enciende** | Dirección I2C errónea o pines invertidos. | Comprobar que SDA esté en GPIO8 y SCL en GPIO9. La dirección por defecto es `0x3C`. Alimentar a 3.3V. |
| **El ESP32 se reinicia al activar la bomba o el servo** | Caída de tensión por pico de arranque inductivo (*Brownout Reset*). | Instalar un condensador electrolítico de $470\,\mu\text{F} / 16\text{V}$ en el riel de potencia de 5V. No alimentar la bomba ni el servo desde el pin 3.3V del ESP32. |
| **El transistor MOSFET se calienta excesivamente** | Se utilizó un MOSFET estándar (ej. IRF540N) en lugar de *Logic-Level*. | Con lógica de 3.3V, un MOSFET tradicional no satura y opera en zona óhmica. Sustituir por **IRLZ44N** o módulo con **AO3400A** cuyo $V_{GS(th)} < 2.0\,\text{V}$. |
| **La bomba sigue sonando o genera ruido parásito** | Falta el diodo flyback en antiparalelo. | Es obligatorio colocar un diodo rápido **1N5819 Schottky** o **1N4007** en bornes directos de la bomba (cátodo a VCC, ánodo al drenador del MOSFET). |
| **El sensor de suelo reporta 0% o 100% fijo** | Disparidad de tolerancias analógicas respecto a la calibración de fábrica. | Ejecutar el procedimiento de calibración de 2 puntos explicado en `docs/calibration_guide.md` para ajustar `CALIB_SOIL_DRY_AIR_RAW` y `CALIB_SOIL_WET_WATER_RAW`. |
| **LM Studio no responde en la consulta de IA** | El servidor local no está iniciado o el puerto no coincide. | En LM Studio, verificar en la pestaña `<->` que el servidor esté en ejecución en el puerto `1234` con CORS activado. |
