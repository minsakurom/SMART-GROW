# SMART-GROW — Plan Progresivo de Pruebas y Validación (6 Niveles)

Este documento establece el protocolo metódico de verificación del invernadero escolar, cubriendo desde los componentes aislados hasta la interacción con modelos de lenguaje locales y pruebas de contingencia de seguridad.

---

## Estado Actual de Ejecución

- [x] **Pruebas de Software Unitarias y de Integración**: Ejecutadas y Superadas (100% OK, 9 pruebas automatizadas en `tests/`).
- [ ] **Pruebas Físicas en Banco de Trabajo**: Pendientes de ejecución en el hardware real del laboratorio escolar por el equipo humano.

---

## Nivel 1: Pruebas de Componentes (Aislados)

### 1.1 Sensor Capacitivo de Suelo (GPIO1)
- **Procedimiento**:
  1. Alimentar con 3.3V del ESP32-S3 y conectar señal a GPIO1 (ADC1_CH0).
  2. Medir en aire seco: verificar lectura ADC estable entre 2800 y 3100.
  3. Sumergir hasta la línea blanca en agua: verificar descenso a 1200 - 1450.
- **Criterio de Aceptación**: Respuesta monotónica decreciente sin saltos erráticos. Diferencia mínima $> 1200$ cuentas de ADC.

### 1.2 Sensor Ambiental DHT22 (GPIO4)
- **Procedimiento**:
  1. Conectar con resistencia pull-up de 4.7 kΩ a 3.3V.
  2. Verificar lecturas cada 2 segundos.
  3. Soplar suavemente aire tibio para comprobar incremento inmediato en humedad relativa y temperatura.
- **Criterio de Aceptación**: Sin retorno de `NaN`. Humedad y temperatura dentro de tolerancias (±2% RH, ±0.5 °C).

### 1.3 Sensor de Luz LDR (GPIO2)
- **Procedimiento**:
  1. Conectar divisor de tensión (LDR a 3.3V, resistencia fija 10 kΩ a GND).
  2. Cubrir con la mano: ADC $< 300$. Iluminar con linterna del móvil: ADC $> 3200$.
- **Criterio de Aceptación**: Rango dinámico $> 2500$ cuentas en el ADC de 12 bits.

### 1.4 Pantalla OLED SSD1306 (GPIO8 SDA, GPIO9 SCL)
- **Procedimiento**:
  1. Escaneo I2C en `0x3C`.
  2. Despliegue del splash screen y los 4 cuadrantes de métricas.
- **Criterio de Aceptación**: Tasa de refresco fluida a 400 kHz sin artefactos visuales.

---

## Nivel 2: Pruebas de Comunicación

### 2.1 Bus I2C
- **Prueba**: Medición con osciloscopio o analizador lógico de flancos en SDA/SCL.
- **Criterio**: Tiempos de subida $t_r < 300\,\text{ns}$ con pull-ups de 4.7 kΩ.

### 2.2 Conectividad Wi-Fi
- **Prueba**: Asociación al AP local en banda 2.4 GHz.
- **Criterio**: Conexión establecida en $< 8\,\text{s}$ con RSSI $>-70\,\text{dBm}$.

### 2.3 API REST Local
- **Prueba**: Petición `GET /api/telemetry` mediante `curl`.
- **Criterio**: Respuesta HTTP 200 con JSON válido en $< 80\,\text{ms}$.

---

## Nivel 3: Pruebas de Actuadores y Potencia

### 3.1 Etapa MOSFET de la Bomba (GPIO14)
- **Prueba**:
  1. Conectar bomba de agua al riel de potencia con diodo 1N5819 en bornes.
  2. Activar compuerta con pulso de 3.3V desde GPIO14.
  3. Monitorear con osciloscopio el pico de apagado en el Drenador del MOSFET.
- **Criterio**: El diodo flyback debe limitar la sobretensión a un máximo de $V_{CC} + 0.4\,\text{V}$. Cero caídas bruscas en el riel 3.3V del microcontrolador.

### 3.2 Etapa Ventilador Térmico (GPIO15)
- **Prueba**: Variación de PWM entre 0% y 100% a 25 kHz.
- **Criterio**: Ausencia total de zumbido audible en la conmutación; arranque suave a partir de 25% de ciclo de trabajo.

### 3.3 Servomotor SG90 (GPIO17)
- **Prueba**: Desplazamiento escalonado: $0^\circ \to 45^\circ \to 90^\circ \to 0^\circ$.
- **Criterio**: Apertura suave de la ventana sin que el servo entre en bloqueo mecánico (*stall*) ni reinicie el ESP32.

---

## Nivel 4: Pruebas de Integración y Lazo Automático

### 4.1 Lazo de Control con Histéresis
- **Prueba**:
  - Estado seco (humedad $< 35\%$): Bomba inicia por 8 segundos.
  - El riego se detiene automáticamente al cumplirse el tiempo o alcanzar 60%.
  - Transición a Cooldown (180s): la bomba no puede volver a activarse aunque se fuerce la lectura baja.
- **Resultado en Software**: VALIDADO en `mock_esp32_server.py`.

### 4.2 Botones de Demostración Escolar
- **Prueba**:
  - Presionar botón de sequía (GPIO5): la pantalla marca `SIM` y se ejecuta riego demostrativo.
  - Presionar botón de calor (GPIO6): ventilador se enciende y servo abre ventana a 90°.
  - Transcurridos 45 segundos: el sistema cancela la simulación y regresa al estado real.
- **Resultado en Software**: VALIDADO en `SimulationManager.cpp`.

---

## Nivel 5: Integración con Inteligencia Artificial Local (LM Studio)

### 5.1 Enlace con Servidor OpenAI Compatible
- **Prueba**: Envío de resumen de telemetría a `http://localhost:1234/v1/chat/completions`.
- **Criterio**: Recepción de análisis agronómico coherente en JSON en $< 4\,\text{s}$ utilizando modelo local cuantizado.

### 5.2 Formato y Consistencia de Respuestas
- **Prueba**: Inspección del JSON retornado por LM Studio (campos `analysis`, `action_needed`, `proposed_action`, `educational_tip`).
- **Criterio**: El puente extrae limpiamente las claves sin fallos de parseo.

---

## Nivel 6: Pruebas de Seguridad y Resiliencia (Failsafe)

| Escenario de Falla | Comportamiento Esperado | Verificación de Seguridad |
|---|---|---|
| **Desconexión del sensor de suelo (cable suelto)** | ADC sube a $>3600$. Sistema entra en `STATE_SENSOR_FAULT`. Bomba bloqueada permanentemente. | ✅ Imposible inundar el cultivo por fallo de sensor. |
| **Pérdida de enlace Wi-Fi** | El lazo de control local en el ESP32 continúa funcionando autónomamente a 10 Hz sin congelar el procesador. | ✅ La planta no depende de la red para sobrevivir. |
| **Comando Malicioso o Inyección en IA ("Regar por 300 segundos")** | La capa `DeterministicSafetyGuard` clampea la duración al máximo de 8 s o deniega la orden si hay cooldown. | ✅ Aprobado y probado en `test_safety_guard.py`. |
| **Petición de riego con suelo saturado** | La capa determinista rechaza el comando con mensaje explicativo. | ✅ Aprobado y probado en `test_safety_guard.py`. |
| **Dispositivo ESP32 apagado o desconectado** | El Dashboard muestra banner rojo de desconexión; no se inventan valores ni se permite enviar comandos. | ✅ Implementado en `app.js` y `index.html`. |
| **Pulsador de Emergencia presionado** | Todos los MOSFETs a nivel BAJO (0V), servo a 45° pasivo, pantalla OLED en alerta. | ✅ Implementado en firmware y mock. |
