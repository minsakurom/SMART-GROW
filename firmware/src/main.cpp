#include <Arduino.h>
#include "../config/GreenhouseConfig.h"
#include "../include/SensorsManager.h"
#include "../include/ActuatorsManager.h"
#include "../include/SimulationManager.h"
#include "../include/StateMachine.h"
#include "../include/DisplayManager.h"
#include "../include/NetworkManager.h"

// Instanciación de los subsistemas modulares
SensorsManager    sensors;
ActuatorsManager  actuators;
SimulationManager simulation;
StateMachine      stateMachine(sensors, actuators, simulation);
DisplayManager    displayManager;
NetworkManager    networkManager(sensors, actuators, simulation, stateMachine);

void setup() {
    // 1. Inicialización de comunicación serial de depuración y telemetría
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=======================================================");
    Serial.println("  SMART-GROW — Invernadero Inteligente Escolar");
    Serial.println("  Placa: LONELY BINARY ESP32-S3 N16R8 Gold Edition");
    Serial.println("  Firmware v1.0.0 — Modos AUTO / MANUAL / SIMULACION");
    Serial.println("=======================================================\n");

    // 2. Inicialización de actuadores en estado seguro (Bomba OFF, Ventana cerrada)
    actuators.begin();

    // 3. Inicialización de sensores y calibración base
    sensors.begin();

    // 4. Inicialización de pulsadores de simulación escolar (GPIO5 y GPIO6)
    simulation.begin();

    // 5. Inicialización de la pantalla OLED SSD1306 (GPIO8 SDA, GPIO9 SCL)
    displayManager.begin();

    // 6. Inicialización de la máquina de estados
    stateMachine.begin();

    // 7. Inicialización de conectividad de red Wi-Fi y servidor REST API
    networkManager.begin(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS);

    Serial.println("[SETUP] Todos los subsistemas inicializados correctamente.");
}

void loop() {
    // 1. Muestreo y filtrado de sensores (no bloqueante, cadencia configurable)
    sensors.update();

    // 2. Procesamiento de pulsadores y temporizador de simulación escolar
    simulation.update(sensors.getData());

    // 3. Evaluación de reglas de negocio, histéresis y transiciones de estado
    stateMachine.update();

    // 4. Gestión de temporizadores de hardware de actuadores (auto-apagado de bomba)
    actuators.update();

    // 5. Actualización de la interfaz visual OLED
    displayManager.update(sensors.getData(),
                          actuators.getStatus(),
                          simulation.getState(),
                          stateMachine,
                          networkManager.getIpAddress(),
                          networkManager.isConnected());

    // 6. Atención a clientes HTTP REST API y emisión de telemetría serial
    networkManager.update();

    // Pequeño retardo de descanso para ceder tiempo al watchdog y tareas de fondo del RTOS
    delay(10);
}
