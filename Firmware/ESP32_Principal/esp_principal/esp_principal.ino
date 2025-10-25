// Arquivo: firmware/ESP32_Principal/ESP32_Principal.ino (Parsing CSV - Corrigido String Error)

// Incluindo arquivos de cabeçalho (DEVE ESTAR NA MESMA PASTA)
#include "ble_uuids.h" 
#include "data_struct.h" 

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#include <cstring> 
#include <stdio.h> // Necessário para sscanf

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

// Variável que armazena o estado ativo e será lida pelo Site.
EmdrConfigData_t latestConfig = {
    .durationPerSideMs = 1000, 
    .intensityPercent = 100, 
    .command = 0 // Inicia parado
}; 

// =========================================================
// 2. LÓGICA DE VALIDAÇÃO DE COMANDO E ATUALIZAÇÃO DE STATUS
// =========================================================

void updateStatusCharacteristic() {
    // Atualiza a Característica de Status com o último estado
    if (pStatusCharacteristic) {
        // Envia a struct binária (para futura implementação, mantendo o tamanho)
        pStatusCharacteristic->setValue((uint8_t*)&latestConfig, CONFIG_DATA_SIZE);
    }
}

void handleCommand(EmdrConfigData_t config) {
    
    latestConfig = config; 
    updateStatusCharacteristic();
    
    const char* status = (config.command == 1) ? "VIBRANDO" : "PARADO";
    
    Serial.println("=============================================");
    Serial.println(">>> RECEBIMENTO E VALIDAÇÃO DE DADOS (CSV) <<<");
    Serial.printf("Status: %s\n", status);
    Serial.printf("Duração por lado (ms): %u\n", latestConfig.durationPerSideMs);
    Serial.printf("Intensidade (0-100%%): %u\n", latestConfig.intensityPercent);
    Serial.println("=============================================");

    digitalWrite(LED_PIN, (config.command == 1) ? HIGH : LOW);
}

// =========================================================
// 3. CALLBACKS DO SERVIDOR BLE (PARSING CSV)
// =========================================================

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Cliente (Site Web BLE) conectado.");
    updateStatusCharacteristic(); 
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Cliente (Site Web BLE) desconectado.");
  }
};

class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    
    // OBTEM O PONTEIRO RAW E O TAMANHO
    const char* rxBuffer = (const char*)pCharacteristic->getData();
    size_t rxLength = pCharacteristic->getValue().length();
    
    // Constrói explicitamente a std::string a partir do buffer C-style (CORRIGINDO O ERRO)
    std::string rxValue(rxBuffer, rxLength); 
    
    if (rxValue.length() > 0) { 
        
        uint32_t durationTemp, intensityTemp, commandTemp;
        
        // Tenta extrair 3 valores separados por vírgula
        int numParsed = sscanf(rxValue.c_str(), "%u,%u,%u", 
                               &durationTemp, &intensityTemp, &commandTemp);
        
        if (numParsed == 3) {
            EmdrConfigData_t incomingConfig; 
            
            // Converte os uint32 temporários para os tipos da struct
            incomingConfig.durationPerSideMs = (uint16_t)durationTemp;
            incomingConfig.intensityPercent = (uint8_t)intensityTemp;
            incomingConfig.command = (uint8_t)commandTemp;
            
            handleCommand(incomingConfig); 
        } else {
            Serial.printf("Erro: Parsing CSV falhou. Recebido: %s\n", rxValue.c_str());
        }
    }
  }
};

// =========================================================
// 4. CONFIGURAÇÃO (SETUP)
// =========================================================

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32 Principal para Validação de Dados (CSV)...");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 

  BLEDevice::init(DEVICE_NAME_FIXO); 
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 1. STATUS CHAR
  pStatusCharacteristic = pService->createCharacteristic(
                      STATUS_CHAR_UUID, 
                      BLECharacteristic::PROPERTY_READ   | 
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
  pStatusCharacteristic->addDescriptor(new BLE2902());
  pStatusCharacteristic->setValue( (uint8_t*) &latestConfig, CONFIG_DATA_SIZE ); 

  // 2. COMMAND CHAR
  pCommandCharacteristic = pService->createCharacteristic(
                      COMMAND_CHAR_UUID, 
                      BLECharacteristic::PROPERTY_WRITE
                    );
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
        BLEDevice::getAdvertising()->stop(); 
        delay(500); 
        pServer->startAdvertising(); 
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("Site Conectado.");
    }
    
    // Lógica de Notificação (Mantida)
    if (deviceConnected) {
        pStatusCharacteristic->setValue(String(value).c_str());
        pStatusCharacteristic->notify();
        value++;
        delay(3000); 
    }
    
    delay(10); 
}