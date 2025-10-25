// Arquivo: firmware/ESP32_Principal/ESP32_Principal.ino (Modo Síncrono Local)

// Bibliotecas e Headers
#include "data_struct.h" 
#include "Adafruit_DRV2605.h" 

#include "ble_uuids.h" 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#include <cstring> 
#include <stdio.h> 

// =========================================================
// 1. DEFINIÇÕES GLOBAIS E DRIVERS
// =========================================================

// Variáveis de Servidor BLE (Site)
BLEServer* pServer = NULL;
BLECharacteristic* pCommandCharacteristic = NULL; 
BLECharacteristic* pStatusCharacteristic = NULL; 
bool deviceConnected = false; 
bool oldDeviceConnected = false; 
uint32_t value = 0; 

// Configuração BLE Servidor (Site)
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define COMMAND_CHAR_UUID   "19b10002-e8f2-537e-4f6c-d104768a1214"
#define STATUS_CHAR_UUID    "19b10001-e8f2-537e-4f6c-d104768a1214"
#define DEVICE_NAME_FIXO    "ESP32" 

// Configuração DRV2605L (Motor Local)
const uint8_t DRV_ADDR_M2   = 0x5A;     
Adafruit_DRV2605 drv_m2; 

// Variáveis de Controle
EmdrConfigData_t currentConfig = {1000, 100, 0}; 
bool isRunning = false;
unsigned long pulseDuration = 0; 
unsigned long lastToggleTime = 0;
bool isMotorActive = false; // Estado LIGADO/DESLIGADO do motor local
const int LED_PIN = 8; 

// REMOVIDO: Tudo relacionado ao Escravo BLE (Cliente)
// Estas variáveis e funções foram removidas para atender ao seu pedido.

// =========================================================
// 2. FUNÇÕES DE CONTROLE DRV2605L (Motor Local)
// =========================================================

void setupDRV(Adafruit_DRV2605& drv, uint8_t addr) {
    if (!drv.begin()) { 
        Serial.printf("ERRO: DRV2605L não encontrado (Endereço 0x%X)\n", addr);
        return;
    }
    Serial.printf("DRV2605L encontrado e inicializado em 0x%X\n", addr);
    drv.selectLibrary(1); 
    drv.setMode(DRV2605_MODE_REALTIME);
    drv.setRealtimeValue(0);
}

void setMotorIntensity(Adafruit_DRV2605& drv, uint8_t intensity) {
    uint8_t duty = map(intensity, 0, 100, 0, 255); 
    drv.setRealtimeValue(duty);
}

void updateStatusCharacteristic() {
    if (pStatusCharacteristic) {
        pStatusCharacteristic->setValue((uint8_t*)&currentConfig, CONFIG_DATA_SIZE);
    }
}

void stopStimulation() {
    isRunning = false;
    setMotorIntensity(drv_m2, 0); // Desliga Motor Local
    digitalWrite(LED_PIN, LOW);
    Serial.println("-> [PRINC]: Estimulação PARADA.");
}

// =========================================================
// 3. LÓGICA DE GATEWAY (Servidor BLE)
// =========================================================

void handleCommand(EmdrConfigData_t config) {
    currentConfig = config; 
    updateStatusCharacteristic();
    
    Serial.println("=============================================");
    Serial.printf(">>> Site Enviou: Duração %lu, Int %u, Cmd %u\n", 
                  currentConfig.durationPerSideMs, config.intensityPercent, config.command);
    Serial.println("=============================================");

    if (config.command == 1 && currentConfig.intensityPercent > 0) {
        
        isRunning = true;
        digitalWrite(LED_PIN, HIGH);
        
        pulseDuration = currentConfig.durationPerSideMs; 
        lastToggleTime = millis(); 
        
        isMotorActive = true; // Inicia o estado como LIGADO
        setMotorIntensity(drv_m2, currentConfig.intensityPercent); // LIGA o Motor
        
        Serial.println("-> [PRINC]: Ciclo Síncrono INICIADO. Estado: LIGADO.");
        
    } else {
        stopStimulation(); // Se for comando 0, simplesmente para tudo.
    }
}

// =========================================================
// 4. CALLBACKS E SETUP
// =========================================================

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; updateStatusCharacteristic(); };
  void onDisconnect(BLEServer* pServer) { 
    deviceConnected = false; 
    Serial.println("Site desconectado."); 
    stopStimulation(); 
  }
};

class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    const char* rxBuffer = (const char*)pCharacteristic->getData();
    size_t rxLength = pCharacteristic->getValue().length();
    std::string rxValue(rxBuffer, rxLength); 
    
    if (rxValue.length() > 0) { 
        uint32_t durationTemp, intensityTemp, commandTemp;
        int numParsed = sscanf(rxValue.c_str(), "%u,%u,%u", &durationTemp, &intensityTemp, &commandTemp);
        
        if (numParsed == 3) {
            EmdrConfigData_t incomingConfig; 
            incomingConfig.durationPerSideMs = durationTemp; 
            incomingConfig.intensityPercent = (uint8_t)intensityTemp;
            incomingConfig.command = (uint8_t)commandTemp;
            handleCommand(incomingConfig); 
        }
    }
  }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando ESP32 Principal (Servidor BLE)...");
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); 
    
    setupDRV(drv_m2, DRV_ADDR_M2);

    // 4.3. Configuração BLE (Servidor)
    BLEDevice::init(DEVICE_NAME_FIXO); 
    pServer = BLEDevice::createServer(); 
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // CONFIG_DATA_SIZE deve ser 6
    pStatusCharacteristic = pService->createCharacteristic( STATUS_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY );
    pStatusCharacteristic->addDescriptor(new BLE2902());
    pStatusCharacteristic->setValue( (uint8_t*) &currentConfig, CONFIG_DATA_SIZE ); 

    pCommandCharacteristic = pService->createCharacteristic( COMMAND_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE );
    pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
    pCommandCharacteristic->addDescriptor(new BLE2902()); 
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID); 
    BLEDevice::startAdvertising();
}

// =========================================================
// 5. LOOP PRINCIPAL (PULSO SÍNCRONO LOCAL)
// =========================================================

void loop() {
    // 5.1. Lógica do Ciclo Síncrono (Alternância)
    if (isRunning && pulseDuration > 0) {
        if (millis() - lastToggleTime >= pulseDuration) {
            
            isMotorActive = !isMotorActive; // Alterna o estado (LIGADO <-> DESLIGADO)
            
            if (isMotorActive) {
                // Alternou para LIGADO
                setMotorIntensity(drv_m2, currentConfig.intensityPercent);
                Serial.println("[PRINC]: Alternando para LIGADO.");
            } else {
                // Alternou para DESLIGADO (Repouso)
                setMotorIntensity(drv_m2, 0); 
                Serial.println("[PRINC]: Alternando para DESLIGADO (Repouso).");
            }
            
            lastToggleTime = millis(); // Reseta o cronômetro para a próxima alternância
        }
    }

    // 5.2. Lógica de Reconexão do Servidor
    if (!deviceConnected && oldDeviceConnected) {
        BLEDevice::getAdvertising()->stop(); 
        delay(500); 
        pServer->startAdvertising(); 
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) oldDeviceConnected = deviceConnected;
    
    if (deviceConnected) {
        // Envia valor de teste apenas para demonstrar atividade
        pStatusCharacteristic->setValue(String(value).c_str());
        pStatusCharacteristic->notify();
        value++;
    }
    
    delay(10); 
}