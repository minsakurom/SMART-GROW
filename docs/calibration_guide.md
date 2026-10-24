# SMART-GROW — Guía Rigurosa de Calibración de Sensores

## 1. Justificación Metrológica del Sensor Capacitivo de Humedad del Suelo

Los sensores capacitivos de humedad (modelos genéricos v1.2 / v2.0) basan su funcionamiento en un temporizador 555 o circuito RC en alta frecuencia. La placa de circuito impreso actúa como las placas de un condensador donde el sustrato circundante es el dieléctrico:
- La constante dieléctrica del aire seco es $\kappa_{aire} \approx 1$.
- La constante dieléctrica del agua pura es $\kappa_{agua} \approx 80$.
- Al aumentar la presencia de agua, aumenta la capacitancia $C$, lo que reduce la frecuencia del oscilador y disminuye la tensión de salida analógica rectificada hacia el ADC del ESP32-S3.

> **Importante para la Feria Científica**:
> La lectura reportada es un **Índice Relativo de Humedad Volumétrica Calibrado (%)** y no un porcentaje gravimétrico absoluto de laboratorio. Debe documentarse como tal para mantener el rigor experimental.

---

## 2. Protocolo de Calibración de 2 Puntos en Hardware Real

Cada sonda capacitiva presenta tolerancias de fabricación de hasta un $\pm 15\%$ en sus componentes pasivos. Para obtener lecturas consistentes es obligatorio realizar la calibración individual:

### Paso 1: Medición de Referencia en Seco ($ADC_{dry}$)
1. Limpiar cuidadosamente la superficie de la sonda con un paño seco.
2. Suspender la sonda en el aire a temperatura ambiente, sin tocar ningún objeto ni el borde de la mesa con la mano.
3. Abrir el monitor serial a 115200 baudios y observar el campo `soil_raw_adc` en la telemetría.
4. Tomar al menos 30 lecturas consecutivas y calcular el promedio.
5. El valor típico del ESP32-S3 (ADC 12 bits, atenuación 11dB) suele situarse entre **2800 y 3100**.
6. Anotar este valor como `CALIB_SOIL_DRY_AIR_RAW`.

### Paso 2: Medición de Referencia en Saturación Hídrica ($ADC_{wet}$)
1. Llenar un vaso con agua destilada o agua desmineralizada a temperatura ambiente.
2. Sumergir la sonda exactamente hasta la **línea blanca de advertencia de inmersión máxima** marcada en la PCB.
   - *Precaución*: Jamás sumergir los componentes electrónicos superiores (el chip 555 ni las resistencias SMD) para evitar daño permanente.
3. Esperar 15 segundos a que la lectura se estabilice.
4. Registrar el promedio de 30 muestras de `soil_raw_adc`.
5. El valor típico en agua suele situarse entre **1200 y 1450**.
6. Anotar este valor como `CALIB_SOIL_WET_WATER_RAW`.

---

## 3. Función de Transferencia y Conversión

La fórmula matemática implementada en `SensorsManager.cpp` es:

$$\text{HumedadRelativa}(\%) = \left( \frac{ADC_{dry} - ADC_{leido}}{ADC_{dry} - ADC_{wet}} \right) \times 100\%$$

Con delimitación rígida de seguridad:
- Si $ADC_{leido} \ge ADC_{dry} \implies \text{Humedad} = 0.0\%$
- Si $ADC_{leido} \le ADC_{wet} \implies \text{Humedad} = 100.0\%$
- Si $ADC_{leido} < 800$ o $ADC_{leido} > 3600 \implies$ **Estado de Error / Sonda Desconectada**.

---

## 4. Calibración del Sensor de Iluminación (LDR GL5528)

El divisor de tensión está formado por la fotorresistencia en serie con una resistencia fija de $R_{fija} = 10\,\text{k}\Omega$:
- En oscuridad: $R_{LDR} \approx 1\,\text{M}\Omega \implies V_{out} \approx 3.3 \times \frac{10\,\text{k}\Omega}{1000\,\text{k}\Omega + 10\,\text{k}\Omega} \approx 0.03\,\text{V} \implies ADC \approx 40$.
- Con luz ambiente de aula (300 lux): $R_{LDR} \approx 10\text{--}15\,\text{k}\Omega \implies ADC \approx 1600\text{--}2100$.
- Con luz solar directa o lámpara LED cercana: $R_{LDR} \approx 1\,\text{k}\Omega \implies ADC \approx 3400\text{--}3700$.

Los valores de referencia en `GreenhouseConfig.h`:
```cpp
#define CALIB_LIGHT_DARK_RAW     250   // 0% luz útil
#define CALIB_LIGHT_BRIGHT_RAW   3600  // 100% saturación lumínica
```

---

## 5. Calibración Mecánica del Servomotor SG90 (Ventana)

1. Desconectar el brazo plástico del estriado del eje del servo antes de encender el equipo.
2. Iniciar el firmware; este enviará automáticamente el comando de posición cerrada ($0^\circ$).
3. Con el servo posicionado en $0^\circ$, colocar el brazo plástico en la posición mecánica de ventana cerrada y fijar el tornillo de sujeción.
4. Al activar el modo ventilación, el servo girará hasta $90^\circ$, empujando la varilla de articulación para abrir la trampilla superior sin forzar el motor contra los topes mecánicos.
