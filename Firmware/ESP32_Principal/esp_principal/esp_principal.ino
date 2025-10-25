// Arquivo: firmware/ESP32_Principal/ESP32_Principal.ino (Corrigido o Buffer de Escrita)

// Incluindo arquivos de cabeçalho (DEVE ESTAR NA MESMA PASTA)
#include "ble_uuids.h" 
#include "data_struct.h" 

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#include <cstring> 

// =========================================================
// 1. VARIÁVEIS GLOBAIS E DEFINIÇÕES
// =========================================================

// CONFIGURAÇÃO SINCRONIZADA COM O CÓDIGO DO SITE
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define COMMAND_CHAR_UUID   "19b10002-e8f2-537e-4f6c-d104768a1214"
#define STATUS_CHAR_UUID    "19b10001-e8f2-537e-4f6c-d104768a1214"
#define DEVICE_NAME_FIXO    "ESP32" 

BLEServer* pServer = NULL;
BLECharacteristic* pCommandCharacteristic = NULL; 
BLECharacteristic* pStatusCharacteristic = NULL; 
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0; 
const int LED_PIN = 8; 
EmdrConfigData_t latestConfig; 

// =========================================================
// 2. LÓGICA DE VALIDAÇÃO DE COMANDO
// =========================================================

void handleCommand(EmdrConfigData_t config) {
    
    const char* status = (config.command == 1) ? "VIBRANDO" : "PARADO";
    
    Serial.println("=============================================");
    Serial.println(">>> RECEBIMENTO E VALIDAÇÃO DE DADOS <<<");
    Serial.printf("Status: %s\n", status);
    Serial.printf("Duração por lado (ms): %u\n", config.durationPerSideMs);
    Serial.printf("Intensidade (0-100%%): %u\n", config.intensityPercent);
    Serial.println("=============================================");

    digitalWrite(LED_PIN, (config.command == 1) ? HIGH : LOW);

    latestConfig = config; 
}

// =========================================================
// 3. CALLBACKS DO SERVIDOR BLE
// =========================================================

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Cliente (Site Web BLE) conectado.");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Cliente (Site Web BLE) desconectado.");
  }};

class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    
    const char* rxBuffer = pCharacteristic->getValue().c_str(); 
    size_t rxLength = pCharacteristic->getValue().length();
    
    if (rxLength == CONFIG_DATA_SIZE) { 
        EmdrConfigData_t incomingConfig; 
        memcpy(&incomingConfig, rxBuffer, CONFIG_DATA_SIZE);
        
        handleCommand(incomingConfig); 
    } else {
        Serial.printf("Erro: Recebido %d bytes, esperado %d bytes. Ignorando.\n", 
                      rxLength, CONFIG_DATA_SIZE);
    }
  }
};

// =========================================================
// 4. CONFIGURAÇÃO (SETUP)
// =========================================================

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32 Principal para Validação de Dados...");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 

  BLEDevice::init(DEVICE_NAME_FIXO); // "ESP32"
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 1. STATUS CHAR
  pStatusCharacteristic = pService->createCharacteristic(
                      STATUS_CHAR_UUID, 
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pStatusCharacteristic->addDescriptor(new BLE2902());

  // 2. COMMAND CHAR
  pCommandCharacteristic = pService->createCharacteristic(
                      COMMAND_CHAR_UUID, 
                      BLECharacteristic::PROPERTY_WRITE
                    );
  
  // *** CORREÇÃO CRÍTICA: DEFINIR TAMANHO DO BUFFER DE ESCRITA ***
  // Inicializa o valor e define o tamanho máximo para 4 bytes.
  pCommandCharacteristic->setValue( (uint8_t*) &latestConfig, CONFIG_DATA_SIZE ); 
  
  pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
  pCommandCharacteristic->addDescriptor(new BLE2902()); 

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); 
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  
  BLEDevice::startAdvertising();
  Serial.println("Servidor BLE pronto e anunciando como 'ESP32'...");
}

// =========================================================
// 5. LOOP PRINCIPAL
// =========================================================

void loop() {
    // Lógica de Reconexão e Notificação (Mantida)
    if (!deviceConnected && oldDeviceConnected) {
        Serial.println("Site desconectado. Reiniciando anúncio.");
        BLEDevice::getAdvertising()->stop(); // Limpa o anúncio antes de recomeçar
        delay(500); 
        pServer->startAdvertising(); 
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("Site Conectado.");
    }

    if (deviceConnected) {
        pStatusCharacteristic->setValue(String(value).c_str());
        pStatusCharacteristic->notify();
        value++;
        delay(3000); 
    }
    
    delay(10); 
}