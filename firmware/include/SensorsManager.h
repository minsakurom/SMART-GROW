#ifndef SENSORS_MANAGER_H
#define SENSORS_MANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include "../config/GreenhouseConfig.h"

struct SensorData {
    float temperature;         // Grados Celsius (°C)
    float airHumidity;         // Humedad relativa del aire (% RH)
    float soilMoisturePct;     // Humedad calibrada del suelo (% relativo)
    float lightLevelPct;       // Nivel de iluminación relativo (%)
    uint16_t soilRawADC;       // Lectura ADC cruda (0 - 4095)
    uint16_t lightRawADC;      // Lectura ADC cruda (0 - 4095)
    
    // Indicadores de salud de sensores
    bool dhtValid;             // True si la lectura de DHT22 es válida
    bool soilSensorValid;      // True si el sensor de suelo está en rango eléctrico
    bool lightSensorValid;     // True si el LDR está en rango
    bool hasFault;             // True si cualquier sensor crítico reporta falla
    String faultMessage;       // Descripción técnica de la falla detectada
};

class SensorsManager {
public:
    SensorsManager();
    void begin();
    bool update(bool force = false);
    const SensorData& getData() const;

    // Métodos para actualizar calibración dinámica
    void setSoilCalibration(uint16_t dryRaw, uint16_t wetRaw);
    void setLightCalibration(uint16_t darkRaw, uint16_t brightRaw);

private:
    DHT _dht;
    SensorData _data;
    unsigned long _lastReadTime;

    uint16_t _calibSoilDry;
    uint16_t _calibSoilWet;
    uint16_t _calibLightDark;
    uint16_t _calibLightBright;

    uint16_t readOversampledADC(uint8_t pin, uint8_t samples = 16);
    float calculateSoilMoisture(uint16_t rawAdc);
    float calculateLightLevel(uint16_t rawAdc);
};

#endif // SENSORS_MANAGER_H
