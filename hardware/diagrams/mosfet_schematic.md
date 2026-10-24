# SMART-GROW — Esquema Eléctrico de las Etapas MOSFET

## 1. Etapa de Conmutación para Cargas Inductivas (Bomba y Ventilador)

Las electrobombas sumergibles de corriente continua y los ventiladores brushless son inductancias. Cuando el MOSFET corta bruscamente la corriente ($di/dt$), la inductancia genera un voltaje inverso $V = -L \frac{di}{dt}$ que puede superar fácilmente los 50 V y destruir la juntura drenador-fuente del MOSFET.

### Circuito Esquemático de Protección Completa

```
   +VCC_EXT (+5V o +12V Fuente Externa)
         |
         +--------------------+
         |                    |
       +---+                [ + ]
       | D | Cátodo         [   ]  CARGA INDUCTIVA
       | I | 1N5819         [   ]  (Bomba DC o Ventilador DC)
       | O | Schottky       [ - ]
       | D | Ánodo            |
       +---+                  |
         |                    |
         +---------+----------+
                   |
                D  |  Drenador
               +---+
   ESP32 GPIO  | N-Channel MOSFET (Logic-Level: IRLZ44N / AO3400A)
   (3.3V Logic)|
        |      |
   +----+--+   |
   |  Rg   |---+  Compuerta (Gate)
   | 150 Ω |   |
   +-------+   |
        |      +---+
        |          |
        |        +----+
        +--------|R_pd| Pulldown 10 kΩ
                 |    | (Mantiene MOSFET apagado en reset/boot)
                 +----+
                   |
                S  |  Fuente (Source)
                   |
   GND_POTENCIA ---+==================================== COMMON GND
```

### Componentes de la Etapa:
1. **MOSFET Logic-Level**:
   - Modelos recomendados: **IRLZ44N** (TO-220, $V_{GS(th)} \approx 1.0\text{--}2.0\,\text{V}$, soporta hasta 47 A, saturación plena a 3.3V) o **AO3400A** (SOT-23, $V_{GS(th)} \approx 0.65\text{--}1.2\,\text{V}$, $I_D = 5.7\,\text{A}$).
   - *Nota de advertencia*: NO emplear IRF540N ni IRF520 genéricos sin etapa previa de transistor, ya que su $V_{GS}$ para saturación completa es de 10 V. A 3.3 V solo conducen una fracción de corriente y disipan calor excesivo.
2. **Resistencia de Compuerta ($R_g = 150\,\Omega$)**:
   - Limita el pico de corriente de carga capacitiva de la compuerta ($C_{iss} \approx 1700\,\text{pF}$) a $I_{peak} = \frac{3.3\,\text{V}}{150\,\Omega} = 22\,\text{mA}$, protegiendo el puerto de salida del ESP32-S3.
3. **Resistencia Pull-Down ($R_{pd} = 10\,\text{k}\Omega$)**:
   - Deriva a masa cualquier carga electrostática cuando el ESP32 está en estado de inicialización o apagado.
4. **Diodo Flyback (1N5819 o 1N4007)**:
   - Conectado en paralelo inverso a la carga. Conduce la corriente recirculante cuando el transistor se apaga, clampeando el voltaje a $V_{CC} + 0.4\,\text{V}$.

---

## 2. Etapa de Conmutación para Iluminación LED (Carga Resistiva)

Para la tira o módulos de LEDs para cultivo (White + Red 660nm spectrum), el circuito es similar pero no requiere diodo flyback al ser una carga no inductiva, optimizado para modulación PWM de alta frecuencia (5 kHz) sin parpadeo visible (*flicker-free*).

```
   +VCC_EXT (+5V o +12V Fuente Externa)
         |
       [ + ]
       [   ]  TIRA / MÓDULO LED FOTOSINTÉTICO
       [   ]  (Con resistencias limitadoras integradas)
       [ - ]
         |
      D  |  Drenador
       +---+
       |   | N-Channel Logic-Level MOSFET
       |
  Rg   |
 150 Ω |---+  Gate
 --[===]---+
           |
         +----+
         |R_pd| 10 kΩ a GND
         +----+
           |
      S  |  Source
         |
   GND --+================================ COMMON GND
```

---

## 3. Etapa de Control del Servomotor SG90

El servomotor SG90 es un actuador de posición accionado por señal PWM de 50 Hz ($T = 20\,\text{ms}$), donde un pulso de $1.0\,\text{ms}$ corresponde a $0^\circ$ (ventana cerrada) y $2.0\,\text{ms}$ a $90^\circ$ (ventana abierta).

```
   +5V_EXT (Fuente Externa de 5 V - Mínimo 1 A pico)
      |
      +-----> Cable ROJO (+5V) del Servo SG90
              |
              +--[ 470 µF / 16V Condensador Electrolítico ]-- GND (Absorbe caídas de tensión)

   ESP32-S3 GPIO17 (3.3V PWM)
      |
      +----[ 220 Ω ]----> Cable NARANJA/AMARILLO (PWM Signal) del Servo SG90

   COMMON GND
      |
      +-----------------> Cable MARRÓN/NEGRO (GND) del Servo SG90
```

> **Validación Eléctrica de Señal**: El circuito interno del SG90 cuenta con un comparador de entrada cuya tensión umbral $V_{IH}$ es de aproximadamente $1.8\,\text{V}$ a $2.0\,\text{V}$. Por tanto, la señal de $3.3\,\text{V}$ del ESP32-S3 activa el servo de forma fiable sin requerir traslador de nivel (*level shifter*), siempre y cuando el pin de masa sea común.
