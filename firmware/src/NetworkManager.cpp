#include "../include/NetworkManager.h"

NetworkManager::NetworkManager(SensorsManager& sensors, 
                               ActuatorsManager& actuators, 
                               SimulationManager& simulation, 
                               StateMachine& sm)
    : _sensors(sensors),
      _actuators(actuators),
      _simulation(simulation),
      _sm(sm),
      _server(WEB_SERVER_PORT),
      _lastWifiCheck(0),
      _lastTelemetryEmit(0),
      _ssid(DEFAULT_WIFI_SSID),
      _password(DEFAULT_WIFI_PASS)
{
}

void NetworkManager::begin(const char* ssid, const char* password) {
    _ssid = ssid;
    _password = password;

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEFAULT_DEVICE_NAME);
    WiFi.begin(_ssid.c_str(), _password.c_str());

    Serial.printf("[WIFI] Conectando a red: %s ...\n", _ssid.c_str());

    setupRoutes();
    _server.begin();
    Serial.println("[HTTP] Servidor REST API activo en puerto 80.");
}

void NetworkManager::sendCorsHeaders() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

void NetworkManager::handleOptions() {
    sendCorsHeaders();
    _server.send(204);
}

void NetworkManager::setupRoutes() {
    // Manejador de preflight CORS para navegadores modernos
    _server.on("/api/telemetry", HTTP_OPTIONS, std::bind(&NetworkManager::handleOptions, this));
    _server.on("/api/actuators", HTTP_OPTIONS, std::bind(&NetworkManager::handleOptions, this));
    _server.on("/api/mode", HTTP_OPTIONS, std::bind(&NetworkManager::handleOptions, this));
    _server.on("/api/simulate", HTTP_OPTIONS, std::bind(&NetworkManager::handleOptions, this));
    _server.on("/api/emergency", HTTP_OPTIONS, std::bind(&NetworkManager::handleOptions, this));

    // Rutas operativas
    _server.on("/api/telemetry", HTTP_GET, std::bind(&NetworkManager::handleGetTelemetry, this));
    _server.on("/api/actuators", HTTP_POST, std::bind(&NetworkManager::handlePostActuators, this));
    _server.on("/api/mode", HTTP_POST, std::bind(&NetworkManager::handlePostMode, this));
    _server.on("/api/simulate", HTTP_POST, std::bind(&NetworkManager::handlePostSimulate, this));
    _server.on("/api/emergency", HTTP_POST, std::bind(&NetworkManager::handlePostEmergency, this));

    _server.onNotFound([this]() {
        sendCorsHeaders();
        _server.send(404, "application/json", "{\"error\":\"Ruta no encontrada\"}");
    });
}

void NetworkManager::handleGetTelemetry() {
    sendCorsHeaders();

    const SensorData& s = _sensors.getData();
    const ActuatorStatus& a = _actuators.getStatus();
    const SimulationState& sim = _simulation.getState();

    StaticJsonDocument<1024> doc;
    doc["device"] = DEFAULT_DEVICE_NAME;
    doc["uptime_sec"] = millis() / 1000;
    doc["mode"] = _sm.getModeString();
    doc["state"] = _sm.getStateString();

    // Bloque Sensores Reales
    JsonObject realObj = doc.createNestedObject("sensors_real");
    realObj["temperature_c"] = serialized(String(s.temperature, 1));
    realObj["humidity_air_pct"] = serialized(String(s.airHumidity, 1));
    realObj["soil_moisture_pct"] = serialized(String(s.soilMoisturePct, 1));
    realObj["light_level_pct"] = serialized(String(s.lightLevelPct, 1));
    realObj["soil_raw_adc"] = s.soilRawADC;
    realObj["light_raw_adc"] = s.lightRawADC;
    realObj["dht_valid"] = s.dhtValid;
    realObj["soil_valid"] = s.soilSensorValid;
    realObj["has_fault"] = s.hasFault;
    realObj["fault_message"] = s.faultMessage;

    // Bloque Sensores Efectivos (alimentan la lógica)
    JsonObject effObj = doc.createNestedObject("sensors_effective");
    effObj["temperature_c"] = serialized(String(_simulation.getEffectiveTemperature(s.temperature), 1));
    effObj["humidity_air_pct"] = serialized(String(_simulation.getEffectiveAirHumidity(s.airHumidity), 1));
    effObj["soil_moisture_pct"] = serialized(String(_simulation.getEffectiveSoilMoisture(s.soilMoisturePct), 1));
    effObj["light_level_pct"] = serialized(String(s.lightLevelPct, 1));

    // Bloque Actuadores
    JsonObject actObj = doc.createNestedObject("actuators");
    actObj["pump_active"] = a.pumpActive;
    actObj["pump_cooldown_sec"] = a.cooldownRemainingSeconds;
    actObj["fan_active"] = a.fanActive;
    actObj["fan_speed_pct"] = a.fanSpeedPct;
    actObj["light_active"] = a.lightActive;
    actObj["light_duty_pct"] = a.lightDutyPct;
    actObj["window_angle_deg"] = a.windowAngle;

    // Bloque Simulación
    JsonObject simObj = doc.createNestedObject("simulation");
    simObj["is_active"] = sim.active;
    simObj["drought_mode"] = sim.droughtMode;
    simObj["heat_mode"] = sim.heatMode;
    simObj["trigger_source"] = sim.triggerSource;

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void NetworkManager::handlePostActuators() {
    sendCorsHeaders();

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Cuerpo JSON requerido\"}");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"JSON malformado\"}");
        return;
    }

    // Si está en modo AUTO y no se solicita cambio explícito a MANUAL, rechazar comando directo
    if (_sm.getMode() == MODE_AUTO) {
        _server.send(403, "application/json", "{\"error\":\"Rechazado: El sistema esta en modo AUTOMATICO. Cambie a MANUAL primero.\"}");
        return;
    }

    // Procesamiento seguro de comandos
    if (doc.containsKey("pump")) {
        bool pumpOn = doc["pump"].as<bool>();
        if (pumpOn) {
            uint32_t dur = doc.containsKey("duration_ms") ? doc["duration_ms"].as<uint32_t>() : PUMP_MAX_RUN_TIME_MS;
            bool ok = _actuators.requestPumpStart(dur, "REST_API_MANUAL");
            if (!ok) {
                _server.send(429, "application/json", "{\"error\":\"Bomba bloqueada por periodo de enfriamiento de sustrato o ya activa\"}");
                return;
            }
        } else {
            _actuators.stopPump("REST_API_MANUAL_STOP");
        }
    }

    if (doc.containsKey("fan")) {
        bool fanOn = doc["fan"].as<bool>();
        uint8_t spd = doc.containsKey("fan_speed") ? doc["fan_speed"].as<uint8_t>() : 100;
        _actuators.setFan(fanOn, spd);
    }

    if (doc.containsKey("light")) {
        uint8_t duty = doc["light"].as<uint8_t>();
        _actuators.setLight(duty);
    }

    if (doc.containsKey("window")) {
        uint8_t angle = doc["window"].as<uint8_t>();
        _actuators.setWindowAngle(angle);
    }

    _server.send(200, "application/json", "{\"status\":\"Comandos ejecutados exitosamente con validacion de seguridad\"}");
}

void NetworkManager::handlePostMode() {
    sendCorsHeaders();
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Cuerpo JSON requerido\"}");
        return;
    }

    StaticJsonDocument<256> doc;
    deserializeJson(doc, _server.arg("plain"));
    String targetMode = doc["mode"].as<String>();

    if (targetMode.equalsIgnoreCase("AUTO")) {
        _sm.setMode(MODE_AUTO);
        _server.send(200, "application/json", "{\"status\":\"Modo AUTOMATICO activado\"}");
    } else if (targetMode.equalsIgnoreCase("MANUAL")) {
        _sm.setMode(MODE_MANUAL);
        _server.send(200, "application/json", "{\"status\":\"Modo MANUAL activado\"}");
    } else {
        _server.send(400, "application/json", "{\"error\":\"Modo desconocido (use AUTO o MANUAL)\"}");
    }
}

void NetworkManager::handlePostSimulate() {
    sendCorsHeaders();
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Cuerpo JSON requerido\"}");
        return;
    }

    StaticJsonDocument<256> doc;
    deserializeJson(doc, _server.arg("plain"));

    if (doc.containsKey("drought")) {
        _simulation.toggleDrought("REST_API");
    }
    if (doc.containsKey("heat")) {
        _simulation.toggleHeat("REST_API");
    }
    if (doc.containsKey("stop") && doc["stop"].as<bool>()) {
        _simulation.stopSimulation();
    }

    _server.send(200, "application/json", "{\"status\":\"Estado de simulacion actualizado\"}");
}

void NetworkManager::handlePostEmergency() {
    sendCorsHeaders();
    _sm.triggerManualEmergencyStop();
    _server.send(200, "application/json", "{\"status\":\"PARADA DE EMERGENCIA EJECUTADA\"}");
}

void NetworkManager::emitSerialTelemetry() {
    unsigned long now = millis();
    if (now - _lastTelemetryEmit < INTERVAL_TELEMETRY_EMIT_MS) {
        return;
    }
    _lastTelemetryEmit = now;

    const SensorData& s = _sensors.getData();
    const ActuatorStatus& a = _actuators.getStatus();
    const SimulationState& sim = _simulation.getState();

    StaticJsonDocument<768> doc;
    doc["type"] = "TELEMETRY";
    doc["uptime"] = now / 1000;
    doc["temp"] = serialized(String(s.temperature, 1));
    doc["humidity"] = serialized(String(s.airHumidity, 1));
    doc["soil"] = serialized(String(s.soilMoisturePct, 1));
    doc["light"] = serialized(String(s.lightLevelPct, 1));
    doc["pump"] = a.pumpActive;
    doc["fan"] = a.fanActive;
    doc["light_on"] = a.lightActive;
    doc["window"] = a.windowAngle;
    doc["mode"] = _sm.getModeString();
    doc["state"] = _sm.getStateString();
    doc["sim_active"] = sim.active;
    doc["sim_drought"] = sim.droughtMode;
    doc["sim_heat"] = sim.heatMode;
    doc["fault"] = s.hasFault;

    String output;
    serializeJson(doc, output);
    Serial.println(output); // Salida limpia en una sola línea para el analizador de Python
}

void NetworkManager::update() {
    _server.handleClient();

    // Verificación y reconexión automática periódica de Wi-Fi
    unsigned long now = millis();
    if (now - _lastWifiCheck > 10000) {
        _lastWifiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            // Reintento silencioso sin bloquear el lazo de control crítico
            WiFi.reconnect();
        }
    }

    // Emisión periódica de JSON por el puerto USB-UART serial
    emitSerialTelemetry();
}

bool NetworkManager::isConnected() const {
    return (WiFi.status() == WL_CONNECTED);
}

String NetworkManager::getIpAddress() const {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}
