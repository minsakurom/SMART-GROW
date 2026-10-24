#include "../include/ActuatorsManager.h"

ActuatorsManager::ActuatorsManager() {
    _status.pumpActive = false;
    _status.pumpStartedAt = 0;
    _status.pumpDurationRequested = 0;
    _status.lastPumpStoppedAt = 0;
    _status.cooldownRemainingSeconds = 0;
    
    _status.fanActive = false;
    _status.fanSpeedPct = 0;
    
    _status.lightActive = false;
    _status.lightDutyPct = 0;
    
    _status.windowAngle = SERVO_WINDOW_CLOSED_ANGLE;
    
    _status.rgbRed = 0;
    _status.rgbGreen = 0;
    _status.rgbBlue = 0;
}

void ActuatorsManager::begin() {
    // 1. Configuración del Pin de la Bomba (GPIO digital con pull-down)
    pinMode(PIN_MOSFET_PUMP, OUTPUT);
    digitalWrite(PIN_MOSFET_PUMP, LOW);

    // 2. Configuración PWM para Ventilador (LEDC)
    ledcSetup(PWM_FAN_CHANNEL, PWM_FAN_FREQ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_MOSFET_FAN, PWM_FAN_CHANNEL);
    ledcWrite(PWM_FAN_CHANNEL, 0);

    // 3. Configuración PWM para Iluminación LED (LEDC)
    ledcSetup(PWM_LIGHT_CHANNEL, PWM_LIGHT_FREQ, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_MOSFET_LIGHT, PWM_LIGHT_CHANNEL);
    ledcWrite(PWM_LIGHT_CHANNEL, 0);

    // 4. Configuración PWM para Servomotor SG90 (LEDC 50 Hz, 14 bits)
    ledcSetup(PWM_SERVO_CHANNEL, PWM_SERVO_FREQ, PWM_SERVO_RESOLUTION_BITS);
    ledcAttachPin(PIN_SERVO_WINDOW, PWM_SERVO_CHANNEL);
    setWindowAngle(SERVO_WINDOW_CLOSED_ANGLE);

    // 5. Configuración LED RGB integrado WS2812B (GPIO48)
    setRgbColor(0, 50, 0); // Verde suave al inicio

    Serial.println("[ACTUATORS] Hardware de potencia y actuadores inicializado con seguridad.");
}

void ActuatorsManager::applyServoPulse(uint8_t degrees) {
    degrees = constrain(degrees, SERVO_WINDOW_CLOSED_ANGLE, SERVO_WINDOW_OPEN_ANGLE);
    _status.windowAngle = degrees;

    // Período = 20 ms = 20,000 µs (f = 50 Hz).
    // Resolución = 14 bits -> 16384 niveles -> 1 nivel ≈ 1.22 µs.
    // SG90 pulso: 0° ≈ 1000 µs, 90° ≈ 2000 µs.
    uint32_t pulseMicros = map(degrees, 0, 90, 1000, 2000);
    uint32_t duty = (pulseMicros * 16384UL) / 20000UL;
    ledcWrite(PWM_SERVO_CHANNEL, duty);
}

void ActuatorsManager::setWindowAngle(uint8_t degrees) {
    applyServoPulse(degrees);
}

bool ActuatorsManager::requestPumpStart(uint32_t durationMs, String source) {
    unsigned long now = millis();

    // Comprobación de seguridad 1: ¿Ya está activa la bomba?
    if (_status.pumpActive) {
        Serial.println("[PUMP_SAFETY] Rechazado: la bomba ya se encuentra en funcionamiento.");
        return false;
    }

    // Comprobación de seguridad 2: Período de enfriamiento (Cooldown)
    if (_status.lastPumpStoppedAt > 0 && (now - _status.lastPumpStoppedAt < PUMP_MIN_COOLDOWN_TIME_MS)) {
        uint32_t waitSec = (PUMP_MIN_COOLDOWN_TIME_MS - (now - _status.lastPumpStoppedAt)) / 1000;
        Serial.printf("[PUMP_SAFETY] Rechazado: En período de enfriamiento de sustrato. Restan %u s.\n", waitSec);
        return false;
    }

    // Comprobación de seguridad 3: Forzar límites máximos temporales
    if (durationMs > PUMP_ABSOLUTE_LIMIT_MS) {
        Serial.printf("[PUMP_SAFETY] Ajustando duración de %u ms al límite absoluto de seguridad (%u ms).\n",
                      durationMs, PUMP_ABSOLUTE_LIMIT_MS);
        durationMs = PUMP_ABSOLUTE_LIMIT_MS;
    }
    if (durationMs == 0) {
        durationMs = PUMP_MAX_RUN_TIME_MS;
    }

    // Activación real del hardware
    _status.pumpActive = true;
    _status.pumpStartedAt = now;
    _status.pumpDurationRequested = durationMs;
    digitalWrite(PIN_MOSFET_PUMP, HIGH);

    Serial.printf("[PUMP] INICIO de riego. Origen: %s | Duración fijada: %u ms\n", source.c_str(), durationMs);
    setRgbColor(0, 0, 100); // Azul en RGB
    return true;
}

void ActuatorsManager::stopPump(String reason) {
    if (_status.pumpActive) {
        digitalWrite(PIN_MOSFET_PUMP, LOW);
        _status.pumpActive = false;
        _status.lastPumpStoppedAt = millis();
        Serial.printf("[PUMP] PARADA de bomba. Motivo: %s\n", reason.c_str());
        setRgbColor(0, 50, 0); // Vuelve a verde
    }
}

void ActuatorsManager::setFan(bool state, uint8_t speedPct) {
    _status.fanActive = state;
    if (!state) {
        _status.fanSpeedPct = 0;
        ledcWrite(PWM_FAN_CHANNEL, 0);
    } else {
        _status.fanSpeedPct = constrain(speedPct, 10, 100);
        uint8_t duty = map(_status.fanSpeedPct, 0, 100, 0, 255);
        ledcWrite(PWM_FAN_CHANNEL, duty);
    }
}

void ActuatorsManager::setLight(uint8_t dutyPct) {
    dutyPct = constrain(dutyPct, 0, 100);
    _status.lightDutyPct = dutyPct;
    _status.lightActive = (dutyPct > 0);
    uint8_t duty = map(dutyPct, 0, 100, 0, 255);
    ledcWrite(PWM_LIGHT_CHANNEL, duty);
}

void ActuatorsManager::setRgbColor(uint8_t r, uint8_t g, uint8_t b) {
    _status.rgbRed = r;
    _status.rgbGreen = g;
    _status.rgbBlue = b;
    // Función integrada en el core ESP32 para el LED RGB de placa
    neopixelWrite(PIN_RGB_LED, r, g, b);
}

void ActuatorsManager::emergencyStopAll() {
    stopPump("EMERGENCY_STOP_TRIGGERED");
    setFan(false, 0);
    setLight(0);
    setWindowAngle(SERVO_WINDOW_VENT_ANGLE); // Posición segura de ventilación pasiva
    setRgbColor(120, 0, 0); // Rojo brillante
    Serial.println("[ACTUATORS] ¡PARADA TOTAL DE EMERGENCIA EJECUTADA!");
}

void ActuatorsManager::update() {
    unsigned long now = millis();

    // 1. Guardián de tiempo de funcionamiento de la bomba
    if (_status.pumpActive) {
        if (now - _status.pumpStartedAt >= _status.pumpDurationRequested) {
            stopPump("AUTO_TIMEOUT_PROTECTION");
        }
    }

    // 2. Cálculo del cooldown restante
    if (_status.lastPumpStoppedAt > 0 && (now - _status.lastPumpStoppedAt < PUMP_MIN_COOLDOWN_TIME_MS)) {
        _status.cooldownRemainingSeconds = (PUMP_MIN_COOLDOWN_TIME_MS - (now - _status.lastPumpStoppedAt)) / 1000;
    } else {
        _status.cooldownRemainingSeconds = 0;
    }
}

const ActuatorStatus& ActuatorsManager::getStatus() const {
    return _status;
}

bool ActuatorsManager::isPumpCooldownActive() const {
    return (_status.cooldownRemainingSeconds > 0);
}
