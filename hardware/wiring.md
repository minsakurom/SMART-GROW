# SMART-GROW — Esquema de Cableado y Conexiones Eléctricas

## 1. Reglas Fundamentales de Seguridad Eléctrica

1. **Separación de Señal y Potencia**:
   - Los GPIO del ESP32-S3 operan estrictamente a **3.3 V** y suministran un máximo de **20 mA**.
   - Ningún motor, electrobomba, servomotor ni tira de LEDs debe ser alimentado directamente desde los pines 3.3V o GPIO del microcontrolador.
2. **Topología de Masa Común (Common GND / Star GND)**:
   - Para que las señales lógicas de control (PWM y ON/OFF) tengan una referencia de voltaje compartida, el terminal **GND de la fuente externa de 5V/12V** y el **GND del ESP32-S3** deben estar sólidamente unidos en un único punto común ("Star Ground").
3. **Protección Inductiva (Diodos Flyback)**:
   - La bomba y el ventilador son cargas fuertemente inductivas. Al desconectarse bruscamente, el colapso del campo magnético genera picos de sobretensión inversa (*back-EMF*) de decenas de voltios.
   - Es obligatorio colocar un diodo rápido (p. ej. **1N5819 Schottky** o **1N4007**) en antiparalelo directo sobre los bornes de la bomba y del ventilador (cátodo a VCC, ánodo a Drenador del MOSFET).
4. **Circuito de Compuerta (Gate) de los MOSFET**:
   - Resistencia en serie de compuerta ($R_g = 150\,\Omega$ a $220\,\Omega$): amortigua picos de corriente capacitiva transitoria que degradan el pin del ESP32.
   - Resistencia pull-down ($R_{pd} = 10\,\text{k}\Omega$ de Gate a GND): asegura que mientras el ESP32 arranca, se reinicia o está en estado de alta impedancia (Hi-Z), la compuerta se mantenga a 0 V y la carga permanezca apagada.

---

## 2. Tabla Pin a Pin de Conexiones

| Origen (ESP32-S3) | Destino (Módulo / Componente) | Tipo de Señal | Nivel Voltaje | Componentes de Protección Requeridos | Observaciones de Compatibilidad Eléctrica |
|---|---|---|---|---|---|
| **GPIO1** | Salida AOUT Sensor Capacitivo Suelo | Analógica In | 0.0 V – 3.0 V | Condensador cerámico 100 nF a GND (filtro paso bajo) | VCC del sensor se conecta a 3.3V para no exceder los 3.3V tolerados en el ADC1_CH0. |
| **GPIO2** | Divisor LDR + Resistencia 10 kΩ | Analógica In | 0.2 V – 3.1 V | Resistencia fija 10 kΩ a GND (divisor con LDR a 3.3V) | Conectado a ADC1_CH1. Con luz alta la resistencia del LDR baja y el voltaje sube hacia 3.3V. |
| **GPIO4** | Pin DATA Sensor DHT22 / AM2302 | Digital Bidireccional | 3.3 V | Resistencia Pull-Up 4.7 kΩ a 3.3V | Si el sensor viene en módulo con PCB, ya incluye la resistencia pull-up. |
| **GPIO5** | Pulsador Simulación Sequía | Entrada Digital | 3.3 V (Pull-up) | Pull-up interna activada + condensador 100 nF a GND | El otro terminal del botón va directo a GND. Activo a nivel BAJO (LOW). |
| **GPIO6** | Pulsador Simulación Calor | Entrada Digital | 3.3 V (Pull-up) | Pull-up interna activada + condensador 100 nF a GND | El otro terminal del botón va directo a GND. Activo a nivel BAJO (LOW). |
| **GPIO8** | Pin SDA Pantalla OLED SSD1306 | Digital I2C (SDA) | 3.3 V | Resistencias pull-up I2C internas o 4.7 kΩ a 3.3V | Bus I2C a 3.3V. El módulo SSD1306 soporta 3.3V directamente. |
| **GPIO9** | Pin SCL Pantalla OLED SSD1306 | Digital I2C (SCL) | 3.3 V | Resistencias pull-up I2C internas o 4.7 kΩ a 3.3V | Frecuencia configurada a 400 kHz. |
| **GPIO14** | Compuerta (Gate) MOSFET Bomba | Digital Salida | 3.3 V | $R_g = 150\,\Omega$ en serie, $R_{pd} = 10\,\text{k}\Omega$ a GND | Usar MOSFET Logic-Level (IRLZ44N o AO3400A). Diodo Flyback en la bomba. |
| **GPIO15** | Compuerta (Gate) MOSFET Ventilador | Salida PWM / Dig | 3.3 V | $R_g = 150\,\Omega$ en serie, $R_{pd} = 10\,\text{k}\Omega$ a GND | Frecuencia PWM de 25 kHz para ventiladores DC silenciosos. Diodo Flyback. |
| **GPIO16** | Compuerta (Gate) MOSFET Iluminación | Salida PWM (LEDC) | 3.3 V | $R_g = 150\,\Omega$ en serie, $R_{pd} = 10\,\text{k}\Omega$ a GND | Conmuta tira LED 12V o 5V. No requiere diodo flyback si la tira es puramente resistiva. |
| **GPIO17** | Cable Señal (Naranja/Blanco) Servo SG90 | Salida PWM (50 Hz) | 3.3 V | Resistencia en serie 220 Ω (opcional) | El SG90 acepta señal de control de 3.3 V. Alimentación VCC debe provenir de 5V externo. |
| **GPIO48** | LED RGB WS2812B en placa | Salida Digital RMT | 3.3 V | Conexión interna de PCB Lonely Binary | No requiere cableado externo. |
| **3V3 Pin** | VCC Sensor Suelo, VCC DHT22, VCC OLED, LDR | Alimentación 3.3V | 3.3 V | Consumo total sensores < 45 mA | Alimentado por el regulador LDO interno del ESP32-S3. |
| **5V0 Pin** | Salida 5V Regulador DC-DC externo | Alimentación Placa | 5.0 V | Condensador electrolítico 470 µF / 16V en riel 5V | Si se alimenta por USB de desarrollo, este pin provee 5V del puerto USB. |
| **GND Pin** | Riel Central de Masa Común | Tierra (0V) | 0.0 V | Conexión en estrella con GND de fuente externa | Retorno común de todas las señales analógicas y de control. |

---

## 3. Matriz de Distribución de Alimentación y Consumos

| Subsistema / Componente | Riel de Alimentación | Voltaje Nominal | Consumo Típico | Consumo Pico | Fuente Suministradora |
|---|---|---|---|---|---|
| **ESP32-S3 Core + Wi-Fi** | Riel 5V o USB | 5.0 V (entrada LDO) | 120 mA | 450 mA (Tx Wi-Fi) | Adaptador 5V 3A o DC-DC Buck |
| **Sensor Suelo Capacitivo** | Riel 3.3V ESP32 | 3.3 V | 5 mA | 8 mA | LDO 3.3V de ESP32-S3 |
| **Sensor DHT22** | Riel 3.3V ESP32 | 3.3 V | 1.5 mA | 2.5 mA | LDO 3.3V de ESP32-S3 |
| **Sensor LDR + Divisor** | Riel 3.3V ESP32 | 3.3 V | 0.3 mA | 0.33 mA | LDO 3.3V de ESP32-S3 |
| **Pantalla OLED SSD1306** | Riel 3.3V ESP32 | 3.3 V | 15 mA | 25 mA (100% píxeles) | LDO 3.3V de ESP32-S3 |
| **Servomotor SG90** | Riel 5V Potencia | 5.0 V | 100 mA (mov.) | 650 mA (bloqueo) | Fuente Externa 5V (NO del 3.3V) |
| **Bomba de Agua DC (3-6V o 12V)** | Riel 5V o 12V Potencia | 5.0 V / 12.0 V | 350 mA | 700 mA (arranque) | Fuente Externa Potencia |
| **Ventilador DC (5V o 12V)** | Riel 5V o 12V Potencia | 5.0 V / 12.0 V | 180 mA | 300 mA (arranque) | Fuente Externa Potencia |
| **Iluminación LED Cultivo** | Riel 5V o 12V Potencia | 5.0 V / 12.0 V | 400 mA | 800 mA | Fuente Externa Potencia |
| **Total Estimado del Sistema** | Riel Potencia | 5V / 12V | ~1.2 A | ~2.9 A | Fuente recomendada: 5V 4A o 12V 3A + Buck |

---

## 4. Conexiones GND y Prevención de Bucles de Masa

```
   [Fuente Externa 5V/12V]
      (+) VCC_EXT ------------> Riel de Potencia (+) [Actuadores: Bomba, Fan, LED, Servo VCC]
      (-) GND_EXT --+
                    |
                    +=========> NODO CENTRAL DE MASA (Estrella / Terminal Block)
                    |               |
                    |               +---> GND de Cargas de Potencia (Source de los MOSFETs)
                    |               |
                    |               +---> GND del Servomotor SG90
                    |               |
                    |               +---> GND del ESP32-S3 (Pin GND)
                    |                        |
                    |                        +---> GND Señales de Sensores (Suelo, LDR, DHT22)
                    |                        +---> GND Pantalla OLED
                    |                        +---> GND Pulsadores de Simulación
```

> **Aislamiento de Ruido**: Los pulsos inductivos de conmutación de la bomba y el ventilador no deben circular por la pista de masa de los sensores analógicos (GPIO1 y GPIO2). Al conectar el Source de los MOSFET directamente al terminal de masa de la fuente de alimentación externa, las corrientes pesadas quedan confinadas a la rama de potencia.
