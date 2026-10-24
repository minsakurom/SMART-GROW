#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../config/GreenhouseConfig.h"
#include "SensorsManager.h"
#include "ActuatorsManager.h"
#include "SimulationManager.h"
#include "StateMachine.h"

class DisplayManager {
public:
    DisplayManager();
    void begin();
    void update(const SensorData& sensors, 
                const ActuatorStatus& actuators, 
                const SimulationState& simulation, 
                const StateMachine& sm,
                const String& ipAddress,
                bool wifiConnected);

private:
    Adafruit_SSD1306 _display;
    unsigned long _lastRefresh;
    bool _isInitialized;
};

#endif // DISPLAY_MANAGER_H
