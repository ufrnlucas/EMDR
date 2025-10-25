// Arquivo: ble_server.cpp

#include "ble_server.h"
#include <BLE2902.h> // <<< CORREÇÃO: Adicionando o include necessário
#include "ble_uuids.h"
// --- Definição das Classes de Callback ---

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { 
    // CRUCIAL: Força o servidor a continuar anunciando para aceitar outros clientes
    pServer->startAdvertising(); 
    
    deviceConnected = true; 
    Serial.printf("[HUB]: Novo cliente conectado. Total: %d\n", pServer->getConnectedCount());
  };
  
  void onDisconnect(BLEServer* pServer) {
    if (pServer->getConnectedCount() == 0) {
        deviceConnected = false;
        Serial.println("[HUB]: Ultimo cliente desconectado.");
        stopStimulation(); 
        pServer->startAdvertising(); // Garante que ele recomece a anunciar quando estiver vazio
    } else {
        Serial.printf("[HUB]: Cliente desconectado. Restantes: %d\n", pServer->getConnectedCount());
    }
  }
};

class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    const char* rxBuffer = (const char*)pCharacteristic->getData();
    size_t rxLength = pCharacteristic->getValue().length();
    std::string rxValue(rxBuffer, rxLength); 
    
    if (rxValue.length() > 0) {
        uint32_t durationTemp, intensityTemp, commandTemp, modeTemp;
        
        int numParsed = sscanf(rxValue.c_str(), "%u,%u,%u,%u", 
                               &durationTemp, &intensityTemp, &commandTemp, &modeTemp);
        
        if (numParsed == 4) {
            EmdrConfigData_t incomingConfig;
            incomingConfig.durationPerSideMs = durationTemp;
            incomingConfig.intensityPercent = (uint8_t)intensityTemp;
            incomingConfig.command = (uint8_t)commandTemp;
            incomingConfig.actuatorMode = (uint8_t)modeTemp;
            handleCommand(incomingConfig); // Chama a lógica principal
        }
    }
  }
};

// --- Funções de Inicialização ---

void init_ble_server() {
    BLEDevice::init(DEVICE_NAME_FIXO);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Caracteristica de Status/Notificacao
    pStatusCharacteristic = pService->createCharacteristic( STATUS_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY );
    pStatusCharacteristic->addDescriptor(new BLE2902()); // Agora BLE2902 é reconhecido
    
    // Caracteristica de Comando/Escrita
    pCommandCharacteristic = pService->createCharacteristic( COMMAND_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE );
    pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
    pCommandCharacteristic->addDescriptor(new BLE2902()); // Agora BLE2902 é reconhecido
    
    pService->start();

    // Inicia o Advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    BLEDevice::startAdvertising();
    Serial.println("--- BLE Server Inicializado e Anunciando. ---");
}