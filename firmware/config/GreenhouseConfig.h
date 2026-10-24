#ifndef GREENHOUSE_CONFIG_H
#define GREENHOUSE_CONFIG_H

#include <Arduino.h>

// ============================================================================
// ASIGNACIÓN DE PINES (LONELY BINARY ESP32-S3 N16R8 GOLD EDITION)
// ============================================================================
#define PIN_SOIL_ANALOG         1   // ADC1_CH0 - Sensor capacitivo de humedad de suelo
#define PIN_LDR_ANALOG          2   // ADC1_CH1 - Sensor de luz LDR (divisor 10k)
#define PIN_DHT_DATA            4   // Bus digital DHT22 / AM2302
#define PIN_BTN_DROUGHT         5   // Botón simulación sequía (activo LOW)
#define PIN_BTN_HEAT            6   // Botón simulación calor (activo LOW)
#define PIN_OLED_SDA            8   // I2C SDA para SSD1306
#define PIN_OLED_SCL            9   // I2C SCL para SSD1306
#define PIN_MOSFET_PUMP        14   // Compuerta MOSFET Bomba de agua (GPIO)
#define PIN_MOSFET_FAN         15   // Compuerta MOSFET Ventilador (LEDC PWM)
#define PIN_MOSFET_LIGHT       16   // Compuerta MOSFET Iluminación LED (LEDC PWM)
#define PIN_SERVO_WINDOW       17   // Señal PWM Servomotor SG90 (Ventana)
#define PIN_RGB_LED            48   // LED RGB direccionable WS2812B integrado

// ============================================================================
// PARÁMETROS DE CALIBRACIÓN DE SENSORES
// ============================================================================
// Calibración del Sensor Capacitivo de Humedad del Suelo (ADC de 12 bits: 0 - 4095)
// Nota: En sensores capacitivos, valor ALTO = SECO (aire), valor BAJO = HÚMEDO (agua).
#define CALIB_SOIL_DRY_AIR_RAW   2950  // Valor ADC típico en aire seco (0% humedad)
#define CALIB_SOIL_WET_WATER_RAW 1320  // Valor ADC típico sumergido en agua (100% humedad)

// Rango de validez eléctrica del sensor de suelo (Detección de falla / desconexión)
#define SOIL_ADC_MIN_VALID       800   // Por debajo de esto: posible cortocircuito
#define SOIL_ADC_MAX_VALID       3600  // Por encima de esto: sensor desconectado / flotante

// Calibración de Luz LDR (0 - 4095)
#define CALIB_LIGHT_DARK_RAW     250   // Oscuridad casi total (~0%)
#define CALIB_LIGHT_BRIGHT_RAW   3600  // Luz solar intensa directa (~100%)

// ============================================================================
// LÍMITES Y UMBRALES DE AUTOMATIZACIÓN (HISTÉRESIS Y SEGURIDAD)
// ============================================================================
// Control de Riego Automático por Humedad de Suelo (%)
#define SOIL_MOISTURE_START_THRESHOLD  35.0f // Inicia riego si humedad < 35%
#define SOIL_MOISTURE_STOP_THRESHOLD   60.0f // Detiene riego si humedad >= 60%

// Seguridad Estricta de la Bomba de Agua (Hardware Protection)
#define PUMP_MAX_RUN_TIME_MS           8000   // Tiempo MÁXIMO continuo de bomba: 8 segundos
#define PUMP_MIN_COOLDOWN_TIME_MS      180000 // Tiempo mínimo de espera entre riegos: 3 minutos (180 s)
#define PUMP_ABSOLUTE_LIMIT_MS         15000  // Límite físico infranqueable permitido por comando (15 s)

// Control de Ventilación y Temperatura (°C)
#define TEMP_VENT_START_THRESHOLD      28.0f // Inicia ventilación y abre ventana si Temp > 28°C
#define TEMP_VENT_STOP_THRESHOLD       24.5f // Detiene ventilador y cierra ventana si Temp < 24.5°C
#define TEMP_CRITICAL_HIGH             38.0f // Alarma térmica severa

// Control de Humedad Relativa Ambiental (% RH)
#define HUMIDITY_AIR_HIGH_THRESHOLD    85.0f // Abre ventilación para evitar hongos si HR > 85%

// Control de Iluminación Fotosintética (%)
#define LIGHT_START_THRESHOLD          30.0f // Enciende luces si luz ambiente < 30%
#define LIGHT_STOP_THRESHOLD           55.0f // Apaga luces si luz ambiente >= 55%

// Límites de Movimiento del Servomotor SG90 (Apertura de Ventana)
#define SERVO_WINDOW_CLOSED_ANGLE      0     // Grados ventana totalmente cerrada
#define SERVO_WINDOW_VENT_ANGLE        45    // Grados ventilación moderada
#define SERVO_WINDOW_OPEN_ANGLE        90    // Grados ventilación máxima

// ============================================================================
// TEMPORIZADORES Y FRECUENCIAS DE MUESTREO (MILLISECONDS)
// ============================================================================
#define INTERVAL_SENSOR_READ_MS        2000  // Lectura de sensores cada 2 segundos
#define INTERVAL_OLED_REFRESH_MS       1000  // Actualización de pantalla cada 1 segundo
#define INTERVAL_TELEMETRY_EMIT_MS     3000  // Emisión serial/red de telemetría cada 3 segundos
#define INTERVAL_BTN_DEBOUNCE_MS       40    // Tiempo de debouncing de pulsadores
#define INTERVAL_SIMULATION_DURATION_MS 45000 // Duración automática del modo simulación (45s)

// ============================================================================
// PARÁMETROS DE RED WI-FI Y SERVICIOS LOCALES
// ============================================================================
#define DEFAULT_WIFI_SSID              "SMART-GROW-WIFI"
#define DEFAULT_WIFI_PASS              "SmartGrow2026"
#define DEFAULT_DEVICE_NAME            "SMART-GROW-ESP32"
#define WEB_SERVER_PORT                80

// ============================================================================
// CONFIGURACIÓN DE PWM (LEDC ESP32-S3)
// ============================================================================
#define PWM_FAN_CHANNEL                0
#define PWM_LIGHT_CHANNEL              1
#define PWM_SERVO_CHANNEL              2
#define PWM_FAN_FREQ                   25000 // 25 kHz para ventilador DC silencioso
#define PWM_LIGHT_FREQ                 5000  // 5 kHz para iluminación sin parpadeo
#define PWM_SERVO_FREQ                 50    // 50 Hz estándar de servomotores (período 20 ms)
#define PWM_RESOLUTION_BITS            8     // 8 bits (0 - 255) para Fan y Luz
#define PWM_SERVO_RESOLUTION_BITS      14    // 14 bits (0 - 16383) para alta precisión en servo

#endif // GREENHOUSE_CONFIG_H
