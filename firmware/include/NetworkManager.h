#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "../config/GreenhouseConfig.h"
#include "SensorsManager.h"
#include "ActuatorsManager.h"
#include "SimulationManager.h"
#include "StateMachine.h"

class NetworkManager {
public:
    NetworkManager(SensorsManager& sensors, 
                   ActuatorsManager& actuators, 
                   SimulationManager& simulation, 
                   StateMachine& sm);
    
    void begin(const char* ssid = DEFAULT_WIFI_SSID, const char* password = DEFAULT_WIFI_PASS);
    void update();

    bool isConnected() const;
    String getIpAddress() const;
    void emitSerialTelemetry();

private:
    SensorsManager& _sensors;
    ActuatorsManager& _actuators;
    SimulationManager& _simulation;
    StateMachine& _sm;

    WebServer _server;
    unsigned long _lastWifiCheck;
    unsigned long _lastTelemetryEmit;
    String _ssid;
    String _password;

    void setupRoutes();
    void sendCorsHeaders();
    void handleGetTelemetry();
    void handlePostActuators();
    void handlePostMode();
    void handlePostSimulate();
    void handlePostEmergency();
    void handleOptions();
};

#endif // NETWORK_MANAGER_H
