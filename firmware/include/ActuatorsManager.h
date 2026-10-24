#ifndef ACTUATORS_MANAGER_H
#define ACTUATORS_MANAGER_H

#include <Arduino.h>
#include "../config/GreenhouseConfig.h"

struct ActuatorStatus {
    bool pumpActive;
    unsigned long pumpStartedAt;
    unsigned long pumpDurationRequested;
    unsigned long lastPumpStoppedAt;
    uint32_t cooldownRemainingSeconds;
    
    bool fanActive;
    uint8_t fanSpeedPct;
    
    bool lightActive;
    uint8_t lightDutyPct;
    
    uint8_t windowAngle;       // 0° (Cerrada) a 90° (Abierta)
    
    // Indicador RGB
    uint8_t rgbRed;
    uint8_t rgbGreen;
    uint8_t rgbBlue;
};

class ActuatorsManager {
public:
    ActuatorsManager();
    void begin();
    void update();

    // Comandos con validación de seguridad
    bool requestPumpStart(uint32_t durationMs = PUMP_MAX_RUN_TIME_MS, String source = "AUTO");
    void stopPump(String reason = "NORMAL_STOP");
    
    void setFan(bool state, uint8_t speedPct = 100);
    void setLight(uint8_t dutyPct);
    void setWindowAngle(uint8_t degrees);
    void setRgbColor(uint8_t r, uint8_t g, uint8_t b);

    // Parada de emergencia de todos los actuadores
    void emergencyStopAll();

    // Consulta de estados
    const ActuatorStatus& getStatus() const;
    bool isPumpCooldownActive() const;

private:
    ActuatorStatus _status;

    void applyServoPulse(uint8_t degrees);
};

#endif // ACTUATORS_MANAGER_H
