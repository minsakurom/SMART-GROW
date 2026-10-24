# SMART-GROW — Puente de Inteligencia Artificial Local (LM Studio)

## 1. Arquitectura de Integración

```
  +------------------+          +------------------------+          +--------------------+
  | ESP32-S3 (Placa) | <=====>  | Servicio Puente Python | <=====>  |  LM Studio Local   |
  |  Sensores/Bomba  |  Wi-Fi   |  FastAPI / SafetyGuard | HTTP/API |  (Puerto 1234)     |
  +------------------+  o USB   +------------------------+          +--------------------+
                                            ^
                                            | HTTP REST
                                            v
                                +------------------------+
                                |  Dashboard Web Local   |
                                +------------------------+
```

---

## 2. Restricciones Deterministas de Seguridad (Safety Guard)

En **SMART-GROW**, un modelo de lenguaje **jamás tiene control directo sobre los actuadores físicos**:
1. **Modo Observador Estricto (`OBSERVATION_ONLY`)**:
   - Por defecto, la IA solo tiene permiso para inspeccionar la telemetría, explicar los fenómenos botánicos y generar conclusiones para los estudiantes. Cualquier acción de actuador sugerida por el modelo es bloqueada inmediatamente.
2. **Capa Determinista en Modo Supervisado (`SUPERVISED_APPROVAL`)**:
   - Si se activa el modo supervisado, cada propuesta de la IA debe superar un filtro algorítmico matemático:
     - **Tiempo Máximo de Riego**: Si la IA pide regar 60 segundos, la capa de seguridad recorta el tiempo al límite absoluto (máximo 8 segundos).
     - **Período de Enfriamiento (Cooldown)**: Si la bomba se activó hace menos de 3 minutos, el comando se rechaza automáticamente para evitar encharcamiento y asfixia radicular.
     - **Verificación de Sustrato**: Si el sensor de humedad reporta que el suelo ya está húmedo (>35%), la orden de riego se anula sin importar lo que el texto del modelo afirme.
     - **Límites de Ventana**: La apertura está acotada entre 0° y 90°.

---

## 3. Configuración Paso a Paso de LM Studio

### Paso 1: Descarga e Instalación
- Descargar e instalar LM Studio desde [lmstudio.ai](https://lmstudio.ai/) en el ordenador del laboratorio escolar.

### Paso 2: Descargar un Modelo de Lenguaje Ligero
En la barra de búsqueda de LM Studio, descargar uno de los siguientes modelos recomendados (formato GGUF, cuantización Q4_K_M):
- **Llama-3.2-3B-Instruct** (Muy rápido, excelente seguimiento de instrucciones JSON).
- **Qwen2.5-Coder-7B-Instruct** o **Qwen2.5-7B-Instruct** (Excelente razonamiento lógico).
- **Phi-3.5-mini-instruct** (Muy eficiente en ordenadores con pocos recursos).

### Paso 3: Iniciar el Servidor Local
1. En la barra lateral izquierda de LM Studio, hacer clic en el icono **Local Server** (`<->`).
2. En el menú superior desplegable, seleccionar el modelo descargado.
3. Configuración del servidor:
   - **Port**: `1234`
   - **CORS**: Habilitado (On)
4. Presionar el botón verde **Start Server**.

### Paso 4: Verificación por Consola
Abrir un terminal (PowerShell o bash) y ejecutar:
```bash
curl http://localhost:1234/v1/models
```
Debe responder un objeto JSON con la lista de modelos disponibles.

---

## 4. Ejecución del Servicio Puente Python

1. Asegurarse de tener Python 3.10+ instalado.
2. Navegar a la carpeta del puente:
   ```bash
   cd SMART-GROW/ai_bridge
   ```
3. Ejecutar el puente en modo observador:
   ```bash
   python main.py --mode OBSERVATION_ONLY --esp32-url http://192.168.1.150 --port 8000
   ```
   *(Si el ESP32 no está conectado aún, el puente generará datos de diagnóstico emulados para pruebas locales).*
