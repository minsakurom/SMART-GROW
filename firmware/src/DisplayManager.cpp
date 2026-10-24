#include "../include/DisplayManager.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C // Dirección I2C típica para SSD1306

DisplayManager::DisplayManager()
    : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
      _lastRefresh(0),
      _isInitialized(false)
{
}

void DisplayManager::begin() {
    // Inicialización del bus I2C con los pines específicos del ESP32-S3
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL, 400000); // 400 kHz Fast I2C

    if (!_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("[OLED] Error: No se encontró la pantalla SSD1306 en 0x3C. Reintentando...");
        _isInitialized = false;
        return;
    }

    _isInitialized = true;
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(1);
    
    // Splash screen inicial
    _display.setCursor(18, 12);
    _display.setTextSize(2);
    _display.println("SMART-GROW");
    _display.setTextSize(1);
    _display.setCursor(15, 36);
    _display.println("Invernadero S3");
    _display.setCursor(20, 50);
    _display.println("Iniciando...");
    _display.display();
    delay(1000);
}

void DisplayManager::update(const SensorData& sensors, 
                            const ActuatorStatus& actuators, 
                            const SimulationState& simulation, 
                            const StateMachine& sm,
                            const String& ipAddress,
                            bool wifiConnected) 
{
    if (!_isInitialized) {
        // Intento de recuperación si la pantalla no respondió al inicio
        if (_display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            _isInitialized = true;
        } else {
            return;
        }
    }

    unsigned long now = millis();
    if (now - _lastRefresh < INTERVAL_OLED_REFRESH_MS) {
        return;
    }
    _lastRefresh = now;

    _display.clearDisplay();
    _display.setTextSize(1);

    // 1. Barra Superior (Encabezado)
    _display.setCursor(0, 0);
    _display.print("SMART-GROW ");
    
    // Indicador de Modo
    if (simulation.active) {
        _display.print("[SIM]");
    } else if (sm.getMode() == MODE_AUTO) {
        _display.print("[AUTO]");
    } else {
        _display.print("[MAN]");
    }

    // Indicador Wi-Fi
    _display.setCursor(102, 0);
    _display.print(wifiConnected ? "W:OK" : "W:--");

    _display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

    // 2. Bloque de Métricas Clave
    _display.setCursor(0, 13);
    _display.printf("Temp: %4.1fC", sensors.temperature);
    _display.setCursor(72, 13);
    _display.printf("Air: %3.0f%%", sensors.airHumidity);

    _display.setCursor(0, 24);
    if (simulation.droughtMode) {
        _display.print("Suelo: !18%! (SIM)");
    } else {
        _display.printf("Suelo: %4.1f%%", sensors.soilMoisturePct);
    }

    _display.setCursor(0, 35);
    _display.printf("Luz:  %3.0f%%", sensors.lightLevelPct);
    _display.setCursor(72, 35);
    _display.printf("Vent: %2d deg", actuators.windowAngle);

    // 3. Estado de Actuadores (Bomba, Fan, Luz)
    _display.drawLine(0, 46, 127, 46, SSD1306_WHITE);
    _display.setCursor(0, 49);
    _display.printf("P:%s", actuators.pumpActive ? "ON " : "OFF");
    _display.setCursor(44, 49);
    _display.printf("F:%s", actuators.fanActive ? "ON " : "OFF");
    _display.setCursor(88, 49);
    _display.printf("L:%3d%%", actuators.lightDutyPct);

    // 4. Barra Inferior (IP o Alertas)
    _display.setCursor(0, 57);
    if (sensors.hasFault) {
        _display.print("!FALLA: ");
        _display.print(sensors.faultMessage.substring(0, 12));
    } else if (simulation.active) {
        _display.printf("SIMULACION ACTIVA");
    } else if (wifiConnected) {
        _display.print(ipAddress);
    } else {
        _display.print("Sin conexion Wi-Fi");
    }

    _display.display();
}
