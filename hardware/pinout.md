# SMART-GROW — Pinout y Asignación de Pines
## Placa: LONELY BINARY ESP32-S3 N16R8 Gold Edition

### 1. Especificaciones del Microcontrolador
- **MCU**: Espressif ESP32-S3 (Xtensa® dual-core 32-bit LX7 hasta 240 MHz).
- **Memoria Flash**: 16 MB Octal SPI (OPI).
- **PSRAM**: 8 MB Octal SPI (OPI).
- **Voltaje de Operación / Lógica GPIO**: 3.3 V DC (¡Los GPIO NO son tolerantes a 5 V!).
- **Corriente máxima por GPIO**: 20 mA (nominal recomendada <= 12 mA).
- **Corriente máxima total del paquete VDD3P3_CPU/RTC**: ~120 mA.

---

### 2. Disposición Física de Pines (Lonely Binary ESP32-S3 Gold Edition)

```
                       +-----------------------+
                       | [USB-OTG]   [USB-UART]|
                       |       [IPEX ANT]      |
               3V3 [01]|                       |[32] 5V0 (VBUS / Ext 5V)
             GPIO0 [02]| (BOOT / Strapping)    |[31] GPIO43 (U0TXD)
             GPIO1 [03]| ADC1_CH0              |[30] GPIO44 (U0RXD)
             GPIO2 [04]| ADC1_CH1              |[29] GPIO45 (Strapping VDD_SPI)
             GPIO3 [05]| (JTAG/Strapping)      |[28] GPIO46 (Strapping ROM Log)
             GPIO4 [06]| RTC_GPIO4             |[27] GPIO47
             GPIO5 [07]| RTC_GPIO5             |[26] GPIO48 (RGB LED WS2812B)
             GPIO6 [08]| RTC_GPIO6             |[25] GPIO21
             GPIO7 [09]| RTC_GPIO7             |[24] GPIO20 (USB D+)
             GPIO8 [10]| RTC_GPIO8             |[23] GPIO19 (USB D-)
             GPIO9 [11]| RTC_GPIO9             |[22] GPIO18
            GPIO10 [12]| RTC_GPIO10            |[21] GPIO17
            GPIO11 [13]| RTC_GPIO11            |[20] GPIO16
            GPIO12 [14]| RTC_GPIO12            |[19] GPIO15
            GPIO13 [15]| RTC_GPIO13            |[18] GPIO14
               GND [16]| Tierra                |[17] GND
                       +-----------------------+
```

> **Verificación de Pines de Memoria Interna (Flash / PSRAM OPI)**:
> En la versión N16R8, los pines internos GPIO 26 a 32 (y en algunos encapsulados GPIO 33-37) están dedicados al bus OPI Flash y PSRAM de alta velocidad.
> La placa Lonely Binary Gold Edition **no expone** estos pines conflictivos al header externo, garantizando que todos los pines expuestos (GPIO 0-14 y GPIO 14-21, 43-48) sean seguros para periféricos de usuario sin corromper el acceso a memoria.

---

### 3. Matriz de Asignación y Verificación Técnica

| Componente | Pin Asignado | Modo GPIO | Nivel Lógico | Periférico MCU | Justificación y Verificación de Compatibilidad |
|---|---|---|---|---|---|
| **Sensor Capacitivo de Humedad del Suelo** | **GPIO1** | `ANALOG_IN` | 0 – 3.0 V | `ADC1_CH0` | Pertenece a ADC1. A diferencia de ADC2, ADC1 no sufre interferencias ni bloqueos al activar la radio Wi-Fi. Señal filtrada por software. |
| **LDR (Sensor de Luz con divisor 10k)** | **GPIO2** | `ANALOG_IN` | 0.2 – 3.1 V | `ADC1_CH1` | Pertenece a ADC1. No presenta conflicto con arranque ni Wi-Fi. Lectura linealizada relativa. |
| **Sensor Ambiental DHT22 / AM2302** | **GPIO4** | `INPUT_PULLUP` / Bidireccional | 3.3 V | GPIO Digital | Pin seguro libre de strapping. Requiere resistencia pull-up de 4.7 kΩ a 3.3V (la mayoría de módulos la integran). |
| **Botón Simulación Sequía** | **GPIO5** | `INPUT_PULLUP` | 3.3 V (Activo LOW) | RTC_GPIO5 | Pin libre de restricciones. Se activa con debounce por software y pull-up interno/externo a 3.3V. |
| **Botón Simulación Calor** | **GPIO6** | `INPUT_PULLUP` | 3.3 V (Activo LOW) | RTC_GPIO6 | Pin libre de restricciones. Activo a masa con debounce de 30 ms. |
| **OLED SSD1306 (SDA)** | **GPIO8** | `OPEN_DRAIN` / I2C | 3.3 V | Hardware I2C SDA | Asignación estándar de I2C en ESP32-S3. Mantiene niveles 3.3 V directos hacia el controlador SSD1306. |
| **OLED SSD1306 (SCL)** | **GPIO9** | `OPEN_DRAIN` / I2C | 3.3 V | Hardware I2C SCL | Reloj de I2C hasta 400 kHz (Fast Mode). Compatible con el bus I2C del ESP32-S3. |
| **MOSFET Bomba de Agua** | **GPIO14** | `OUTPUT` | 3.3 V | GPIO Digital / PWM | Controla compuerta (Gate) de MOSFET canal N tipo Logic-Level con R_gate (150 Ω) y R_pulldown (10 kΩ a GND). |
| **MOSFET Ventilador** | **GPIO15** | `OUTPUT` (LEDC) | 3.3 V | PWM Timer | Control de velocidad PWM o conmutación ON/OFF directa vía MOSFET Logic-Level con diodo Flyback. |
| **MOSFET Iluminación LED** | **GPIO16** | `OUTPUT` (LEDC) | 3.3 V | PWM Timer | Dimmer de luz de cultivo fotosintética mediante modulación de ancho de pulso a 5 kHz. |
| **Servo SG90 (Apertura Ventana)** | **GPIO17** | `OUTPUT` (LEDC) | 3.3 V (Señal PWM) | PWM (50 Hz / 20 ms) | La señal PWM del SG90 reconoce 3.3 V como nivel alto (`V_IH` mín = 2.0 V). La alimentación (VCC) del servo va a 5 V externa. |
| **LED RGB Integrado en Placa** | **GPIO48** | `OUTPUT` | 3.3 V | RMT / NeoPixel | WS2812B integrado en la placa Lonely Binary. Muestra estados visuales (Verde=Normal, Azul=Riego, Amarillo=Simulación, Rojo=Alarma). |

---

### 4. Análisis de Pines Críticos y Strapping

1. **GPIO0 (Boot)**:
   - Debe mantenerse en nivel **ALTO** durante el arranque para iniciar desde Flash SPI. Si se pone a masa durante el reinicio entra en modo Download UART.
   - *Decisión de diseño*: Se mantiene **DESCONECTADO** de cargas externas y solo accesible mediante el pulsador físico "BOOT" de la placa.
2. **GPIO45 y GPIO46**:
   - GPIO45 controla el voltaje LDO de memoria SPI (`VDD_SPI`). Un nivel erróneo puede causar bajo voltaje en Flash.
   - GPIO46 controla si se imprimen los mensajes de arranque de la ROM.
   - *Decisión de diseño*: Se mantienen **DESCONECTADOS** de cualquier actuador o sensor.
3. **GPIO43 y GPIO44**:
   - Pines de depuración por defecto `U0TXD` y `U0RXD`.
   - *Decisión de diseño*: Reservados para comunicación serial USB-UART de telemetría hacia el ordenador con el puente de IA.
