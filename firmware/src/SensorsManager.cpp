#include "../include/SensorsManager.h"

SensorsManager::SensorsManager() 
    : _dht(PIN_DHT_DATA, DHT22),
      _lastReadTime(0),
      _calibSoilDry(CALIB_SOIL_DRY_AIR_RAW),
      _calibSoilWet(CALIB_SOIL_WET_WATER_RAW),
      _calibLightDark(CALIB_LIGHT_DARK_RAW),
      _calibLightBright(CALIB_LIGHT_BRIGHT_RAW)
{
    _data.temperature = 22.0f;
    _data.airHumidity = 50.0f;
    _data.soilMoisturePct = 50.0f;
    _data.lightLevelPct = 50.0f;
    _data.soilRawADC = 2000;
    _data.lightRawADC = 1800;
    _data.dhtValid = false;
    _data.soilSensorValid = false;
    _data.lightSensorValid = false;
    _data.hasFault = false;
    _data.faultMessage = "Inicializando...";
}

void SensorsManager::begin() {
    _dht.begin();
    
    // Configuración del conversor analógico-digital (ADC) del ESP32-S3
    analogReadResolution(12); // Resolución estándar de 12 bits (0 - 4095)
    // Atenuación de 11 dB para permitir mediciones hasta ~3.1V en 3.3V VDD
    analogSetPinAttenuation(PIN_SOIL_ANALOG, ADC_11db);
    analogSetPinAttenuation(PIN_LDR_ANALOG, ADC_11db);

    // Primera lectura forzada
    update(true);
}

uint16_t SensorsManager::readOversampledADC(uint8_t pin, uint8_t samples) {
    uint32_t accumulator = 0;
    for (uint8_t i = 0; i < samples; i++) {
        accumulator += analogRead(pin);
        delayMicroseconds(150); // Pequeño retardo entre muestras para estabilización del S/H
    }
    return static_cast<uint16_t>(accumulator / samples);
}

float SensorsManager::calculateSoilMoisture(uint16_t rawAdc) {
    // Verificación de integridad eléctrica
    if (rawAdc < SOIL_ADC_MIN_VALID || rawAdc > SOIL_ADC_MAX_VALID) {
        return -1.0f; // Código de error de lectura fuera de rango
    }

    // En sensores capacitivos, mayor ADC = más seco; menor ADC = más húmedo.
    if (_calibSoilDry <= _calibSoilWet) {
        return 0.0f; // Parámetros de calibración inconsistentes
    }

    float percentage = 0.0f;
    if (rawAdc >= _calibSoilDry) {
        percentage = 0.0f;
    } else if (rawAdc <= _calibSoilWet) {
        percentage = 100.0f;
    } else {
        percentage = (static_cast<float>(_calibSoilDry - rawAdc) / 
                      static_cast<float>(_calibSoilDry - _calibSoilWet)) * 100.0f;
    }

    return constrain(percentage, 0.0f, 100.0f);
}

float SensorsManager::calculateLightLevel(uint16_t rawAdc) {
    if (_calibLightBright <= _calibLightDark) {
        return 0.0f;
    }

    float percentage = (static_cast<float>(rawAdc - _calibLightDark) / 
                        static_cast<float>(_calibLightBright - _calibLightDark)) * 100.0f;
    return constrain(percentage, 0.0f, 100.0f);
}

bool SensorsManager::update(bool force) {
    unsigned long now = millis();
    if (!force && (now - _lastReadTime < INTERVAL_SENSOR_READ_MS)) {
        return false;
    }
    _lastReadTime = now;

    // 1. Lectura del DHT22
    float t = _dht.readTemperature();
    float h = _dht.readHumidity();

    if (isnan(t) || isnan(h) || t < -20.0f || t > 70.0f || h < 0.0f || h > 100.0f) {
        _data.dhtValid = false;
        _data.faultMessage = "Falla de comunicación con sensor DHT22 (GPIO4)";
    } else {
        _data.temperature = t;
        _data.airHumidity = h;
        _data.dhtValid = true;
    }

    // 2. Lectura del Sensor Capacitivo de Suelo
    _data.soilRawADC = readOversampledADC(PIN_SOIL_ANALOG, 16);
    float soilPct = calculateSoilMoisture(_data.soilRawADC);
    if (soilPct < 0.0f) {
        _data.soilSensorValid = false;
        _data.faultMessage = "Sensor de suelo fuera de rango eléctrico (desconectado o en corto)";
    } else {
        _data.soilMoisturePct = soilPct;
        _data.soilSensorValid = true;
    }

    // 3. Lectura del LDR
    _data.lightRawADC = readOversampledADC(PIN_LDR_ANALOG, 16);
    _data.lightLevelPct = calculateLightLevel(_data.lightRawADC);
    _data.lightSensorValid = true;

    // Evaluación de Fallas Globales
    if (!_data.dhtValid || !_data.soilSensorValid) {
        _data.hasFault = true;
    } else {
        _data.hasFault = false;
        _data.faultMessage = "OK";
    }

    return true;
}

const SensorData& SensorsManager::getData() const {
    return _data;
}

void SensorsManager::setSoilCalibration(uint16_t dryRaw, uint16_t wetRaw) {
    if (dryRaw > wetRaw) {
        _calibSoilDry = dryRaw;
        _calibSoilWet = wetRaw;
    }
}

void SensorsManager::setLightCalibration(uint16_t darkRaw, uint16_t brightRaw) {
    if (brightRaw > darkRaw) {
        _calibLightDark = darkRaw;
        _calibLightBright = brightRaw;
    }
}
