// Arquivo: ble_server.cpp (Versão Final e Corrigida com strtok)

#include "ble_server.h"
#include <BLE2902.h>
#include <cstring> // Para strncpy, strtok
#include <cstdlib> // Para strtoul, atoi
#include "ble_uuids.h"
#include "data_struct.h"
#include "profile_manager.h" 
#include "global_vars.h" 
#include "emdr_logic.h" 

// --- Definição das Classes de Callback ---

// 1. CALLBACKS DO SERVIDOR (Conexão/Desconexão)
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { 
        pServer->startAdvertising(); 
        deviceConnected = true; 
        Serial.printf("[HUB]: Novo cliente conectado. Total: %d\n", pServer->getConnectedCount());
    };
    
    void onDisconnect(BLEServer* pServer) {
        if (pServer->getConnectedCount() == 0) {
            deviceConnected = false;
            Serial.println("[HUB]: Ultimo cliente desconectado.");
            stopStimulation(); 
            pServer->startAdvertising(); 
        } else {
            Serial.printf("[HUB]: Cliente desconectado. Restantes: %d\n", pServer->getConnectedCount());
        }
    }
};

// 2. CALLBACKS DE COMANDO (Recebimento de Dados do App - USO DE STRTOK)
class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        // Variaveis temporarias
        unsigned long durationTemp = 0; // Usado para strtoul
        uint8_t intensityTemp = 0, commandTemp = 0, modeTemp = 0, profileIdTemp = 0, actionTypeTemp = 0;
        int numParsed = 0; 

        // CRÍTICO: Copiar para um array C mutável para usar strtok
        const char* dataPtr = (const char*)pCharacteristic->getData(); 
        size_t len = pCharacteristic->getValue().length();
        
        // Crie um buffer mutável (char*) para strtok
        char rxBuffer[64]; 
        memset(rxBuffer, 0, 64);
        // strncpy é necessário para criar uma cópia mutável
        strncpy(rxBuffer, dataPtr, (len < 64 ? len : 63)); 

        char* token;
        int index = 0;

        // ************************************************
        // 1. Tokenização Manual (STRTOK)
        // ************************************************
        
        token = strtok(rxBuffer, ",");
        while (token != NULL) {
            switch (index) {
                case 0: durationTemp = strtoul(token, NULL, 10); break; // Garante leitura de unsigned long
                case 1: intensityTemp = (uint8_t)atoi(token); break;
                case 2: commandTemp = (uint8_t)atoi(token); break;
                case 3: modeTemp = (uint8_t)atoi(token); break;
                case 4: profileIdTemp = (uint8_t)atoi(token); break;
                case 5: actionTypeTemp = (uint8_t)atoi(token); break;
            }
            token = strtok(NULL, ",");
            index++;
        }
        numParsed = index;

        // ************************************************
        // DEBUG CRÍTICO: Mostra o que foi lido
        Serial.printf("[DEBUG PARSE]: Conteudo lido: %s\n", rxBuffer);
        Serial.printf("[DEBUG PARSE]: numParsed: %d, Duracao: %lu\n", numParsed, durationTemp);
        // ************************************************
        
        if (numParsed == 6) { 
            EmdrCommandFromApp_t incomingCommand;
            
            // Mapeamento
            incomingCommand.durationPerSideMs = (uint32_t)durationTemp; // Cast (de unsigned long)
            incomingCommand.intensityPercent = intensityTemp;
            incomingCommand.command = commandTemp;
            incomingCommand.actuatorMode = modeTemp;
            incomingCommand.profileId = profileIdTemp;
            incomingCommand.actionType = actionTypeTemp;

            handleCommand(incomingCommand); 
        } else {
            Serial.printf("[HUB]: ERRO de comando BLE. Recebido %d/%d campos. Conteudo (Buffer C): %s\n", numParsed, 6, rxBuffer);
        }
    }
};

// 3. CALLBACKS DE LEITURA (ALL_PROFILES)
class AllProfilesCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) {
        Serial.println("[HUB]: Recebida requisicao de READ para ALL_PROFILES. Carregando 35 bytes...");
        
        EmdrAllProfiles_t allProfilesData;
        loadAllProfiles(&allProfilesData);
        
        pCharacteristic->setValue((uint8_t*)&allProfilesData, sizeof(EmdrAllProfiles_t));
        
        Serial.printf("[HUB]: %d bytes de todos os perfis enviados.\n", sizeof(EmdrAllProfiles_t));
    }
};


// --- Funções de Inicialização ---

void init_ble_server() {
    BLEDevice::init(DEVICE_NAME_FIXO);
    pServer = BLEDevice::createServer(); 
    pServer->setCallbacks(new MyServerCallbacks());

    // 1. SERVIÇO PRINCIPAL (APP <-> PRINCIPAL)
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Caracteristica de Status/Notificacao (7 bytes)
    pStatusCharacteristic = pService->createCharacteristic( 
        STATUS_CHAR_UUID, 
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY 
    );
    pStatusCharacteristic->addDescriptor(new BLE2902()); 
    
    // Caracteristica de Comando/Escrita (9 bytes)
    pCommandCharacteristic = pService->createCharacteristic( 
        COMMAND_CHAR_UUID, 
        BLECharacteristic::PROPERTY_WRITE 
    );
    pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
    pCommandCharacteristic->addDescriptor(new BLE2902()); 
    
    // NOVO: Característica ALL_PROFILES (Leitura em Bloco - 35 bytes)
    pAllProfilesCharacteristic = pService->createCharacteristic(
        ALL_PROFILES_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ 
    );
    pAllProfilesCharacteristic->setCallbacks(new AllProfilesCharacteristicCallbacks());
    
    // Preenche o valor inicial da característica ALL_PROFILES com o conteúdo da EEPROM
    EmdrAllProfiles_t initialProfiles;
    loadAllProfiles(&initialProfiles); 
    pAllProfilesCharacteristic->setValue((uint8_t*)&initialProfiles, sizeof(EmdrAllProfiles_t));


    // 2. SERVIÇO INTERNO (PRINCIPAL <-> AUXILIAR)
    BLEService *pServiceInt = pServer->createService(SERVICE_UUID_INT);
    pAuxCommandCharacteristic = pServiceInt->createCharacteristic(
        CHAR_UUID_SYNC_INT,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    
    pService->start();
    pServiceInt->start();

    // Inicia o Advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID); 
    BLEDevice::startAdvertising();
    Serial.println("--- BLE Server Inicializado e Anunciando. ---");
}