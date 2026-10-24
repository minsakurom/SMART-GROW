#include "../include/StateMachine.h"

StateMachine::StateMachine(SensorsManager& sensors, ActuatorsManager& actuators, SimulationManager& simulation)
    : _sensors(sensors),
      _actuators(actuators),
      _simulation(simulation),
      _mode(MODE_AUTO),
      _state(STATE_BOOTING),
      _stateEnteredAt(0)
{
}

void StateMachine::begin() {
    _stateEnteredAt = millis();
    transitionTo(STATE_MONITORING_NORMAL);
}

void StateMachine::transitionTo(SystemState newState) {
    if (_state == newState) return;
    
    _state = newState;
    _stateEnteredAt = millis();
    Serial.printf("[STATE_MACHINE] Transición a estado: %s (Modo: %s)\n", 
                  getStateString().c_str(), getModeString().c_str());

    // Actualización de señalizaciones visuales LED RGB según estado
    switch (_state) {
        case STATE_MONITORING_NORMAL:
            _actuators.setRgbColor(0, 40, 0); // Verde tranquilo
            break;
        case STATE_IRRIGATING:
            _actuators.setRgbColor(0, 0, 80); // Azul
            break;
        case STATE_VENTILATING:
            _actuators.setRgbColor(0, 60, 60); // Cian
            break;
        case STATE_SENSOR_FAULT:
            _actuators.setRgbColor(120, 20, 0); // Naranja/Rojo de advertencia
            break;
        case STATE_EMERGENCY_SHUTDOWN:
            _actuators.setRgbColor(150, 0, 0); // Rojo brillante
            break;
        default:
            break;
    }
}

void StateMachine::setMode(SystemMode mode) {
    _mode = mode;
    Serial.printf("[STATE_MACHINE] Modo de control cambiado a: %s\n", getModeString().c_str());
    if (_mode == MODE_MANUAL) {
        // En modo manual no forzamos apagar todo, pero nos aseguramos de que no haya ciclos descontrolados
    }
}

SystemMode StateMachine::getMode() const {
    return _mode;
}

SystemState StateMachine::getState() const {
    return _state;
}

String StateMachine::getStateString() const {
    switch (_state) {
        case STATE_BOOTING: return "INICIALIZANDO";
        case STATE_MONITORING_NORMAL: return "NORMAL_ESTABLE";
        case STATE_IRRIGATING: return "REGANDO";
        case STATE_VENTILATING: return "VENTILANDO";
        case STATE_SENSOR_FAULT: return "FALLA_SENSORES";
        case STATE_EMERGENCY_SHUTDOWN: return "PARADA_EMERGENCIA";
        default: return "DESCONOCIDO";
    }
}

String StateMachine::getModeString() const {
    return (_mode == MODE_AUTO) ? "AUTOMATICO" : "MANUAL";
}

void StateMachine::triggerManualEmergencyStop() {
    _actuators.emergencyStopAll();
    transitionTo(STATE_EMERGENCY_SHUTDOWN);
}

void StateMachine::resetEmergency() {
    Serial.println("[STATE_MACHINE] Reiniciando desde estado de emergencia.");
    transitionTo(STATE_MONITORING_NORMAL);
}

void StateMachine::processAutoLogic(float temp, float humidity, float soil, float light) {
    const ActuatorStatus& act = _actuators.getStatus();

    // 1. Control de Riego con Histéresis y Protección Anti-Inundación
    if (soil < SOIL_MOISTURE_START_THRESHOLD) {
        if (!act.pumpActive && !_actuators.isPumpCooldownActive()) {
            Serial.printf("[AUTO] Humedad de suelo baja (%.1f%% < %.1f%%). Iniciando ciclo de riego.\n", 
                          soil, SOIL_MOISTURE_START_THRESHOLD);
            if (_actuators.requestPumpStart(PUMP_MAX_RUN_TIME_MS, "AUTO_SOIL_LOW")) {
                transitionTo(STATE_IRRIGATING);
            }
        }
    } else if (soil >= SOIL_MOISTURE_STOP_THRESHOLD && act.pumpActive) {
        Serial.printf("[AUTO] Humedad alcanzada (%.1f%% >= %.1f%%). Deteniendo bomba.\n", 
                      soil, SOIL_MOISTURE_STOP_THRESHOLD);
        _actuators.stopPump("SOIL_TARGET_REACHED");
    }

    // 2. Control Térmico y Ventilación (Histéresis en Temperatura y Humedad Alta)
    bool needVentilation = (temp > TEMP_VENT_START_THRESHOLD) || (humidity > HUMIDITY_AIR_HIGH_THRESHOLD);
    bool canStopVentilation = (temp < TEMP_VENT_STOP_THRESHOLD) && (humidity < (HUMIDITY_AIR_HIGH_THRESHOLD - 5.0f));

    if (needVentilation) {
        if (!act.fanActive || act.windowAngle < SERVO_WINDOW_OPEN_ANGLE) {
            _actuators.setFan(true, 100);
            _actuators.setWindowAngle(SERVO_WINDOW_OPEN_ANGLE);
            if (_state != STATE_IRRIGATING) {
                transitionTo(STATE_VENTILATING);
            }
        }
    } else if (canStopVentilation) {
        if (act.fanActive || act.windowAngle > SERVO_WINDOW_CLOSED_ANGLE) {
            _actuators.setFan(false, 0);
            _actuators.setWindowAngle(SERVO_WINDOW_CLOSED_ANGLE);
            if (_state == STATE_VENTILATING) {
                transitionTo(STATE_MONITORING_NORMAL);
            }
        }
    }

    // 3. Control de Iluminación Fotosintética Suplementaria (Histéresis de Luz)
    if (light < LIGHT_START_THRESHOLD) {
        if (!act.lightActive) {
            _actuators.setLight(80); // Encender al 80% de potencia
        }
    } else if (light >= LIGHT_STOP_THRESHOLD) {
        if (act.lightActive) {
            _actuators.setLight(0);  // Apagar tira LED
        }
    }

    // Si la bomba terminó de regar y no está ventilando, retornar a monitoreo normal
    if (_state == STATE_IRRIGATING && !act.pumpActive) {
        if (act.fanActive) {
            transitionTo(STATE_VENTILATING);
        } else {
            transitionTo(STATE_MONITORING_NORMAL);
        }
    }
}

void StateMachine::update() {
    const SensorData& rawSensors = _sensors.getData();

    // 1. Detección de Falla Crítica en Sensores
    if (rawSensors.hasFault) {
        if (_state != STATE_SENSOR_FAULT && _state != STATE_EMERGENCY_SHUTDOWN) {
            Serial.printf("[STATE_MACHINE] ¡ALERTA! Sensor en falla: %s\n", rawSensors.faultMessage.c_str());
            _actuators.stopPump("SENSOR_FAULT_FAILSAFE");
            _actuators.setWindowAngle(SERVO_WINDOW_VENT_ANGLE); // Ventilación pasiva segura
            transitionTo(STATE_SENSOR_FAULT);
        }
        return;
    } else if (_state == STATE_SENSOR_FAULT) {
        // Recuperación de la falla
        Serial.println("[STATE_MACHINE] Sensores normalizados. Retomando operación.");
        transitionTo(STATE_MONITORING_NORMAL);
    }

    // Si estamos en Parada de Emergencia, no ejecutar automatismos
    if (_state == STATE_EMERGENCY_SHUTDOWN) {
        return;
    }

    // 2. Extraer lecturas efectivas (reales o simuladas según corresponda)
    float effSoil = _simulation.getEffectiveSoilMoisture(rawSensors.soilMoisturePct);
    float effTemp = _simulation.getEffectiveTemperature(rawSensors.temperature);
    float effHum  = _simulation.getEffectiveAirHumidity(rawSensors.airHumidity);
    float effLight = rawSensors.lightLevelPct;

    // 3. Ejecución del lazo de control según modo
    if (_mode == MODE_AUTO) {
        processAutoLogic(effTemp, effHum, effSoil, effLight);
    }
}
