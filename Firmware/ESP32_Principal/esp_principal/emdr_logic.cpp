// Arquivo: emdr_logic.cpp

#include "emdr_logic.h"

// --- Variaveis Globais (Alocacao de Memoria) ---
BLEServer* pServer = NULL;
BLECharacteristic* pCommandCharacteristic = NULL;
BLECharacteristic* pStatusCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

EmdrConfigData_t currentConfig = {1000, 100, 0, 3};
bool isRunning = false;
unsigned long pulseDuration = 0;
unsigned long lastToggleTime = 0;
bool isLocalActive = false; 

// Constantes
const size_t CONFIG_DATA_SIZE = 7; 
const uint8_t DRV_ADDR_M2   = 0x5A;     
Adafruit_DRV2605 drv_m2;
const int LED_PIN = 1; 


void updateStatusCharacteristic() {
    if (pStatusCharacteristic) {
        pStatusCharacteristic->setValue((uint8_t*)&currentConfig, CONFIG_DATA_SIZE);
        // O valor do contador 'value' é atualizado no loop()
    }
}

void stopStimulation() {
    isRunning = false;
    setMotorIntensity(drv_m2, 0); 
    digitalWrite(LED_PIN, LOW); 
    Serial.println("-> [LOGIC]: Estimulacao PARADA.");
}

void controlActuatorsLocal(bool activate) {
    bool controlMotor = (currentConfig.actuatorMode == 1 || currentConfig.actuatorMode == 3);
    bool controlLED = (currentConfig.actuatorMode == 2 || currentConfig.actuatorMode == 3);
    
    if (activate) {
        if (controlMotor) setMotorIntensity(drv_m2, currentConfig.intensityPercent);
        if (controlLED) digitalWrite(LED_PIN, HIGH);
        Serial.println("[LOGIC]: Lado Local LIGADO.");
    } else {
        if (controlMotor) setMotorIntensity(drv_m2, 0); 
        if (controlLED) digitalWrite(LED_PIN, LOW); 
        Serial.println("[LOGIC]: Lado Local DESLIGADO (Repouso).");
    }
}

void handleCommand(EmdrConfigData_t config) {
    currentConfig = config; 
    
    Serial.printf(">>> Site Enviou: Dur %lu, Int %u, Cmd %u, Modo %u\n", 
                  currentConfig.durationPerSideMs, config.intensityPercent, config.command, config.actuatorMode);

    if (config.command == 1 && currentConfig.intensityPercent > 0) {
        isRunning = true;
        pulseDuration = currentConfig.durationPerSideMs; 
        lastToggleTime = millis(); 
        isLocalActive = true; 
        
        controlActuatorsLocal(true); 
    } else {
        stopStimulation();
    }
}

// --- Funcao do Loop Principal ---
void run_emdr_cycle() {
    // Logica do Ciclo Sincrono (Alternancia LIGADO <-> DESLIGADO)
    if (isRunning && pulseDuration > 0) {
        if (millis() - lastToggleTime >= pulseDuration) {
            
            isLocalActive = !isLocalActive; 
            controlActuatorsLocal(isLocalActive); 
            
            lastToggleTime = millis(); 
        }
    }

    // Logica de Notificacao (atualizacao da caracteristica)
    if (pStatusCharacteristic && deviceConnected) {
        updateStatusCharacteristic();
        pStatusCharacteristic->setValue(String(value++).c_str());
        pStatusCharacteristic->notify();
    }

    // Logica de Reconexao
    if (!deviceConnected && oldDeviceConnected) {
        BLEDevice::getAdvertising()->stop();
        delay(500);
        pServer->startAdvertising();
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) oldDeviceConnected = deviceConnected;
}