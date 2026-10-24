#include "../include/SimulationManager.h"

SimulationManager::SimulationManager() 
    : _lastBtnDroughtPress(0),
      _lastBtnHeatPress(0),
      _lastBtnDroughtState(HIGH),
      _lastBtnHeatState(HIGH)
{
    _state.active = false;
    _state.droughtMode = false;
    _state.heatMode = false;
    _state.startedAt = 0;
    _state.durationMs = INTERVAL_SIMULATION_DURATION_MS;
    _state.triggerSource = "NONE";
}

void SimulationManager::begin() {
    // Los pulsadores se configuran con pull-up interna activa (nivel HIGH en reposo, LOW al presionar)
    pinMode(PIN_BTN_DROUGHT, INPUT_PULLUP);
    pinMode(PIN_BTN_HEAT, INPUT_PULLUP);
    Serial.println("[SIMULATION] Pulsadores de demostración GPIO5 y GPIO6 listos.");
}

void SimulationManager::toggleDrought(String source) {
    if (_state.droughtMode) {
        _state.droughtMode = false;
        if (!_state.heatMode) _state.active = false;
        Serial.println("[SIMULATION] Desactivada simulación de sequía.");
    } else {
        _state.droughtMode = true;
        _state.active = true;
        _state.startedAt = millis();
        _state.triggerSource = source;
        Serial.printf("[SIMULATION] ¡ACTIVADA SIMULACIÓN DE SEQUÍA! (Origen: %s)\n", source.c_str());
    }
}

void SimulationManager::toggleHeat(String source) {
    if (_state.heatMode) {
        _state.heatMode = false;
        if (!_state.droughtMode) _state.active = false;
        Serial.println("[SIMULATION] Desactivada simulación de calor.");
    } else {
        _state.heatMode = true;
        _state.active = true;
        _state.startedAt = millis();
        _state.triggerSource = source;
        Serial.printf("[SIMULATION] ¡ACTIVADA SIMULACIÓN DE OLA DE CALOR! (Origen: %s)\n", source.c_str());
    }
}

void SimulationManager::stopSimulation() {
    _state.active = false;
    _state.droughtMode = false;
    _state.heatMode = false;
    _state.triggerSource = "NONE";
    Serial.println("[SIMULATION] Todas las simulaciones han sido canceladas. Retornando a lecturas reales.");
}

void SimulationManager::checkPhysicalButtons() {
    unsigned long now = millis();

    // 1. Lectura con debouncing del botón de Sequía (GPIO5)
    int readingDrought = digitalRead(PIN_BTN_DROUGHT);
    if (readingDrought == LOW && _lastBtnDroughtState == HIGH && (now - _lastBtnDroughtPress > INTERVAL_BTN_DEBOUNCE_MS)) {
        _lastBtnDroughtPress = now;
        toggleDrought("BUTTON_GPIO5");
    }
    _lastBtnDroughtState = readingDrought;

    // 2. Lectura con debouncing del botón de Calor (GPIO6)
    int readingHeat = digitalRead(PIN_BTN_HEAT);
    if (readingHeat == LOW && _lastBtnHeatState == HIGH && (now - _lastBtnHeatPress > INTERVAL_BTN_DEBOUNCE_MS)) {
        _lastBtnHeatPress = now;
        toggleHeat("BUTTON_GPIO6");
    }
    _lastBtnHeatState = readingHeat;
}

void SimulationManager::update(const SensorData& realSensors) {
    checkPhysicalButtons();

    // Temporizador de desconexión automática del modo simulación por seguridad
    if (_state.active) {
        if (millis() - _state.startedAt > _state.durationMs) {
            Serial.println("[SIMULATION] Tiempo límite de simulación escolar finalizado. Restableciendo normalidad.");
            stopSimulation();
        }
    }
}

float SimulationManager::getEffectiveSoilMoisture(float realSoil) const {
    if (_state.droughtMode) {
        // En simulación de sequía, inyectamos un valor crítico de 18% para activar el ciclo de riego
        return 18.0f;
    }
    return realSoil;
}

float SimulationManager::getEffectiveTemperature(float realTemp) const {
    if (_state.heatMode) {
        // En simulación de calor, inyectamos 34.5 °C para activar ventilación y apertura de ventana
        return 34.5f;
    }
    return realTemp;
}

float SimulationManager::getEffectiveAirHumidity(float realHum) const {
    if (_state.heatMode) {
        // En ola de calor suele bajar la humedad relativa
        return 32.0f;
    }
    return realHum;
}

const SimulationState& SimulationManager::getState() const {
    return _state;
}
