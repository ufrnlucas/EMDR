// Arquivo: firmware/ESP32_Principal/ESP32_Principal.ino (Gateway FINAL)

// Bibliotecas e Headers
#include "data_struct.h" 
#include "Adafruit_DRV2605.h" 

#include "ble_uuids.h" 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEAdvertisedDevice.h> 
#include <string>
#include <cstring> 
#include <stdio.h> 

// =========================================================
// 1. DEFINIÇÕES GLOBAIS E DRIVERS
// =========================================================

// Variáveis Globais (Definidas no topo para garantir o escopo)
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

// Configuração DRV2605L (Ambos DRV2605L usam 0x5A, pois estão em barramentos separados)
const uint8_t DRV_ADDR_M2   = 0x5A;     
Adafruit_DRV2605 drv_m2; // Inicialização padrão (assume 0x5A)

// Variáveis Cliente BLE (Comunicação com o Escravo)
static BLEAdvertisedDevice* pServerEscravo = nullptr; 
static bool doScan = true; 
static bool connectedEscravo = false; 
// Características de Sincronização (UUIDs de ble_uuids.h)
static BLERemoteCharacteristic* pSyncCharacteristic = nullptr; 
static BLEClient* pClientEscravo = nullptr; 

// Estrutura de Pulso para Envio BLE (Intensidade/Comando)
typedef struct { uint8_t intensity; uint8_t command; } MotorPulse_t;
const size_t PULSE_DATA_SIZE = 2; // Tamanho do pulso (Intensidade + Comando)

// Variáveis de Controle
EmdrConfigData_t currentConfig = {1000, 100, 0}; 
bool isRunning = false;
unsigned long lastToggleTime = 0;
bool isMotor1Active = false; // true: Motor 1 (Escravo); false: Motor 2 (Local)
const int LED_PIN = 8; 

// =========================================================
// 2. FUNÇÕES DE CONTROLE DRV2605L E CLIENTE BLE
// =========================================================

void setupDRV(Adafruit_DRV2605& drv, uint8_t addr) {
    // Inicialização final do DRV2605L Local (Motor 2)
    // O endereço é definido via construtor (drv_m2), mas begin() usa o barramento Wire
    if (!drv.begin()) { 
        Serial.printf("ERRO: DRV2605L não encontrado (Endereço 0x%X)\n", addr);
        return;
    }
    Serial.printf("DRV2605L encontrado e inicializado em 0x%X\n", addr);
    drv.selectLibrary(1); 
    drv.setMode(DRV2605_MODE_REALTIME);
}

void setMotorIntensity(Adafruit_DRV2605& drv, uint8_t intensity) {
    uint8_t duty = map(intensity, 0, 100, 0, 255); 
    drv.setRealtimeValue(duty);
}

// Funções de Cliente BLE
bool connectToEscravo();

// Envia o pulso de 2 bytes via BLE para o ESP32 Escravo (Motor 1)
void sendBLEPulse(uint8_t intensity, uint8_t command) {
    if (!connectedEscravo || pSyncCharacteristic == nullptr) {
        // Tenta reconectar ou escanear
        Serial.println("-> [PRINC]: Escravo desconectado. Pulso não enviado.");
        doScan = true;
        return;
    }
    
    MotorPulse_t pulse = {intensity, command};
    
    // Envia o Array de 2 bytes (Intensidade + Comando)
    if (!pSyncCharacteristic->writeValue((uint8_t*)&pulse, PULSE_DATA_SIZE, false)) {
        Serial.println("-> [PRINC]: Falha na escrita BLE. Desconectando Cliente.");
        pClientEscravo->disconnect();
        connectedEscravo = false;
        doScan = true;
    }
}

// =========================================================
// 3. LÓGICA DE GATEWAY (BLE CLIENTE/SERVERS e CICLO)
// =========================================================

void updateStatusCharacteristic() {
    if (pStatusCharacteristic) {
        pStatusCharacteristic->setValue((uint8_t*)&currentConfig, CONFIG_DATA_SIZE);
    }
}

void stopStimulation() {
    isRunning = false;
    setMotorIntensity(drv_m2, 0); // Desliga Motor 2 (Local DRV)
    sendBLEPulse(0, 0); // Envia PARAR para Motor 1 (Escravo)
    digitalWrite(LED_PIN, LOW);
    Serial.println("-> [PRINC]: Estimulação PARADA.");
}

void handleCommand(EmdrConfigData_t config) {
    currentConfig = config; 
    updateStatusCharacteristic();
    
    Serial.println("=============================================");
    Serial.printf(">>> Site Enviou: Duração %u, Int %u, Cmd %u\n", 
                  config.durationPerSideMs, config.intensityPercent, config.command);
    Serial.println("=============================================");

    if (config.command == 1 && currentConfig.intensityPercent > 0) {
        isRunning = true;
        digitalWrite(LED_PIN, HIGH);
        
        isMotor1Active = true; 
        sendBLEPulse(currentConfig.intensityPercent, 1); // Liga Motor 1 (Escravo)
        setMotorIntensity(drv_m2, 0); // Garante Motor 2 Desligado (Local DRV)
        lastToggleTime = millis();
        Serial.println("-> [PRINC]: Ciclo INICIADO. Ativo: Motor 1 (Escravo).");
    } else {
        stopStimulation();
    }
}

// =========================================================
// 4. LÓGICA DE CLIENTE BLE (Scan e Conexão ao Escravo)
// =========================================================

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // DEVICE_NAME_ESCRAVO vem de ble_uuids.h
        if (advertisedDevice.getName() == DEVICE_NAME_ESCRAVO) { 
            advertisedDevice.getScan()->stop(); 
            pServerEscravo = new BLEAdvertisedDevice(advertisedDevice);
            doScan = false; 
        }
    }
};

bool connectToEscravo() {
    if (pServerEscravo == nullptr) return false;

    if (pClientEscravo != nullptr && pClientEscravo->isConnected()) {
        pClientEscravo->disconnect();
        delay(10);
    }
    
    pClientEscravo = BLEDevice::createClient();
    if (!pClientEscravo->connect(pServerEscravo)) return false;

    BLERemoteService* pRemoteService = pClientEscravo->getService(SERVICE_UUID_INT);
    if (pRemoteService == nullptr) {
        pClientEscravo->disconnect();
        return false;
    }

    pSyncCharacteristic = pRemoteService->getCharacteristic(CHAR_UUID_SYNC_INT);
    if (pSyncCharacteristic == nullptr || !pSyncCharacteristic->canWrite()) {
        pClientEscravo->disconnect();
        return false;
    }
    
    connectedEscravo = true;
    return true;
}

// =========================================================
// 5. CALLBACKS E SETUP
// =========================================================

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; updateStatusCharacteristic(); };
  void onDisconnect(BLEServer* pServer) { 
    deviceConnected = false; 
    Serial.println("Site desconectado."); 
    stopStimulation(); // Desliga motores ao desconectar
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
            incomingConfig.durationPerSideMs = (uint16_t)durationTemp;
            incomingConfig.intensityPercent = (uint8_t)intensityTemp;
            incomingConfig.command = (uint8_t)commandTemp;
            handleCommand(incomingConfig); 
        }
    }
  }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando ESP32 Principal (Gateway FINAL)...");
    
    // 5.1. Inicialização de Hardware
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); 
    
    // 5.2. Inicializa o driver DRV2605L Local (Motor 2)
    // O DRV2605L local usa o barramento I2C padrão do ESP32 (GPIO 21/22 se não for definido).
    if (!drv_m2.begin()) { // Tenta inicializar com endereço 0x5A
        Serial.println("ERRO FATAL: DRV2605L Motor 2 não encontrado.");
    }
    setupDRV(drv_m2, DRV_ADDR_M2);

    // 5.3. Configuração BLE (Servidor)
    BLEDevice::init(DEVICE_NAME_FIXO); 
    pServer = BLEDevice::createServer(); 
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pStatusCharacteristic = pService->createCharacteristic( STATUS_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY );
    pStatusCharacteristic->addDescriptor(new BLE2902());
    pStatusCharacteristic->setValue( (uint8_t*) &currentConfig, CONFIG_DATA_SIZE ); 

    pCommandCharacteristic = pService->createCharacteristic( COMMAND_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE );
    pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
    pCommandCharacteristic->addDescriptor(new BLE2902()); 
    pService->start();

    // 5.4. Inicializa o Scanner BLE (para o Escravo)
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID); 
    BLEDevice::startAdvertising();

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); 
    pBLEScan->start(5, false); // Scan inicial de 5s
}

// =========================================================
// 6. LOOP PRINCIPAL
// =========================================================

void loop() {
    // 6.1. Lógica do Ciclo Síncrono
    if (isRunning && currentConfig.durationPerSideMs > 0) {
        if (millis() - lastToggleTime >= currentConfig.durationPerSideMs) {
            
            isMotor1Active = !isMotor1Active;
            
            if (isMotor1Active) {
                // Ativa Motor 1 (Escravo BLE), Desliga Motor 2 (Local DRV)
                setMotorIntensity(drv_m2, 0); 
                sendBLEPulse(currentConfig.intensityPercent, 1); 
            } else {
                // Ativa Motor 2 (Local DRV), Desliga Motor 1 (Escravo BLE)
                setMotorIntensity(drv_m2, currentConfig.intensityPercent);
                sendBLEPulse(0, 0); 
            }
            
            lastToggleTime = millis();
        }
    }

    // 6.2. Lógica de Reconexão BLE (Cliente)
    if (pServerEscravo != nullptr && !connectedEscravo) {
        if (connectToEscravo()) {
            Serial.println("[PRINC]: Cliente BLE conectado ao Escravo.");
        } else {
            Serial.println("[PRINC]: Falha ao conectar ao Escravo. Tentando escanear novamente...");
            doScan = true; 
        }
    } else if (doScan) {
        BLEDevice::getScan()->start(5, false); // Escaneia por 5s
        doScan = false;
        pServerEscravo = nullptr; 
    }

    // 6.3. Reconexão do Servidor e Notificação (Mantida)
    if (!deviceConnected && oldDeviceConnected) {
        BLEDevice::getAdvertising()->stop(); 
        delay(500); 
        pServer->startAdvertising(); 
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) oldDeviceConnected = deviceConnected;
    
    if (deviceConnected) {
        pStatusCharacteristic->setValue(String(value).c_str());
        pStatusCharacteristic->notify();
        value++;
    }
    
    delay(10); 
}