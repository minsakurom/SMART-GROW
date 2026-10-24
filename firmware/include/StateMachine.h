#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include "../config/GreenhouseConfig.h"
#include "SensorsManager.h"
#include "ActuatorsManager.h"
#include "SimulationManager.h"

enum SystemMode {
    MODE_AUTO,
    MODE_MANUAL
};

enum SystemState {
    STATE_BOOTING,
    STATE_MONITORING_NORMAL,
    STATE_IRRIGATING,
    STATE_VENTILATING,
    STATE_SENSOR_FAULT,
    STATE_EMERGENCY_SHUTDOWN
};

class StateMachine {
public:
    StateMachine(SensorsManager& sensors, ActuatorsManager& actuators, SimulationManager& simulation);
    void begin();
    void update();

    void setMode(SystemMode mode);
    SystemMode getMode() const;
    SystemState getState() const;
    String getStateString() const;
    String getModeString() const;

    void triggerManualEmergencyStop();
    void resetEmergency();

private:
    SensorsManager& _sensors;
    ActuatorsManager& _actuators;
    SimulationManager& _simulation;

    SystemMode _mode;
    SystemState _state;
    unsigned long _stateEnteredAt;

    void transitionTo(SystemState newState);
    void processAutoLogic(float temp, float humidity, float soil, float light);
};

#endif // STATE_MACHINE_H
