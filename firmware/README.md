# SMART-GROW — Guía de Compilación y Carga de Firmware

## 1. Características Técnicas del Firmware
- **Arquitectura**: C++ modular no bloqueante orientado a eventos y tareas temporizadas (`millis()`).
- **Placa Objetivo**: LONELY BINARY ESP32-S3 N16R8 Gold Edition (ESP32-S3 con 16MB Flash y 8MB PSRAM OPI).
- **Control**: Máquina de estados finitos con histéresis estricta e interlocks de hardware para la bomba.
- **Conectividad**: Servidor HTTP REST API con soporte CORS y emisión periódica de telemetría serial en formato JSON.

---

## 2. Compilación y Carga con PlatformIO (Recomendado)

### Requisitos:
- Visual Studio Code con la extensión **PlatformIO IDE** o CLI `pio`.

### Pasos:
1. Abrir la carpeta `SMART-GROW/firmware` en VS Code.
2. Conectar el ESP32-S3 al ordenador mediante el puerto **USB-UART** (o USB-OTG según se configure).
3. Abrir la terminal de PlatformIO y compilar:
   ```bash
   pio run
   ```
4. Subir el firmware al microcontrolador:
   ```bash
   pio run --target upload
   ```
5. Abrir el monitor serial para verificar el arranque:
   ```bash
   pio device monitor
   ```

---

## 3. Compilación y Carga con Arduino IDE 2.x

Si prefieres usar **Arduino IDE**:

1. En **Herramientas > Administrador de Placas**, asegúrate de tener instalado el paquete **esp32 by Espressif Systems** (versión 2.0.14 o 3.0+).
2. Seleccionar la placa: **ESP32S3 Dev Module**.
3. Ajustar los parámetros de menú en **Herramientas**:
   - **Flash Size**: 16MB (128Mb)
   - **Partition Scheme**: 16MB Flash (3MB APP / 9.9MB FATFS) o similar
   - **PSRAM**: OPI PSRAM
   - **Flash Mode**: QIO 80MHz o OPI 80MHz
   - **Upload Speed**: 921600
   - **USB Mode**: Hardware CDC and JTAG (o Normal UART)
4. Instalar las bibliotecas requeridas desde el **Gestor de Bibliotecas**:
   - `Adafruit SSD1306`
   - `Adafruit GFX Library`
   - `DHT sensor library`
   - `Adafruit Unified Sensor`
   - `ArduinoJson` (v6 o v7)
5. Abrir el archivo `SMART-GROW/firmware/src/main.cpp` (puedes crear un archivo `.ino` espejo si tu versión de Arduino IDE no admite proyectos puramente `.cpp`).
6. Presionar **Subir**.

---

## 4. Endpoints de la API REST Integrada

| Método | Endpoint | Parámetros / Cuerpo | Descripción |
|---|---|---|---|
| `GET` | `/api/telemetry` | Ninguno | Retorna el estado completo del sistema en JSON (sensores reales, efectivos, actuadores, simulación). |
| `POST` | `/api/actuators` | `{"pump": true, "duration_ms": 5000}` | Control manual de bomba, fan, luz o ventana. Rechazado si el sistema está en modo AUTO. |
| `POST` | `/api/mode` | `{"mode": "AUTO"}` o `{"mode": "MANUAL"}` | Alterna el modo de operación de la máquina de estados. |
| `POST` | `/api/simulate` | `{"drought": true}` o `{"heat": true}` o `{"stop": true}` | Activa o detiene simulaciones para demostraciones escolares. |
| `POST` | `/api/emergency` | Ninguno | Ejecuta una parada total e inmediata de todos los actuadores. |
