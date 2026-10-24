#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H

#include <Arduino.h>
#include "../config/GreenhouseConfig.h"
#include "SensorsManager.h"

struct SimulationState {
    bool active;              // True si alguna simulación está en curso
    bool droughtMode;         // Simulación de sequía severa
    bool heatMode;            // Simulación de ola de calor
    unsigned long startedAt;
    uint32_t durationMs;
    String triggerSource;     // "BUTTON_GPIO5", "BUTTON_GPIO6", "REST_API"
};

class SimulationManager {
public:
    SimulationManager();
    void begin();
    void update(const SensorData& realSensors);

    // Activación / Desactivación manual y por software
    void toggleDrought(String source = "REST_API");
    void toggleHeat(String source = "REST_API");
    void stopSimulation();

    // Obtención de datos efectivos para la máquina de estados
    float getEffectiveSoilMoisture(float realSoil) const;
    float getEffectiveTemperature(float realTemp) const;
    float getEffectiveAirHumidity(float realHum) const;

    const SimulationState& getState() const;

private:
    SimulationState _state;

    // Variables de debounce para los pulsadores físicos
    unsigned long _lastBtnDroughtPress;
    unsigned long _lastBtnHeatPress;
    int _lastBtnDroughtState;
    int _lastBtnHeatState;

    void checkPhysicalButtons();
};

#endif // SIMULATION_MANAGER_H
