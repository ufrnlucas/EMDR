// Arquivo: ble_server.cpp (Versão Limpa)

#include "ble_server.h"
#include <BLE2902.h>
#include <cstring>
#include "ble_uuids.h"
#include "data_struct.h"
#include "profile_manager.h" 
#include "global_vars.h" // Para acessar pServer, pCommandCharacteristic, etc.
#include "emdr_logic.h"  // Para stopStimulation e handleCommand

// --- Definição das Classes de Callback ---

// 1. CALLBACKS DO SERVIDOR (Conexão/Desconexão)
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { 
        // Força o servidor a continuar anunciando para aceitar outros clientes
        pServer->startAdvertising(); 
        deviceConnected = true; 
        Serial.printf("[HUB]: Novo cliente conectado. Total: %d\n", pServer->getConnectedCount());
    };
    
    void onDisconnect(BLEServer* pServer) {
        if (pServer->getConnectedCount() == 0) {
            deviceConnected = false;
            Serial.println("[HUB]: Ultimo cliente desconectado.");
            stopStimulation(); // Para a vibração ao perder a conexão com o último App
            pServer->startAdvertising(); 
        } else {
            Serial.printf("[HUB]: Cliente desconectado. Restantes: %d\n", pServer->getConnectedCount());
        }
    }
};

// 2. CALLBACKS DE COMANDO (Recebimento de Dados do App - 6 Valores)
class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        // As variáveis temporárias são declaradas dentro desta função
        uint32_t durationTemp; 
        uint8_t intensityTemp, commandTemp, modeTemp, profileIdTemp, actionTypeTemp;
        int numParsed = 0; 

        const char* rxBuffer = (const char*)pCharacteristic->getData();
        size_t rxLength = pCharacteristic->getValue().length();
        std::string rxValue(rxBuffer, rxLength); 
        
        if (rxValue.length() > 0) {
            // Tenta ler 6 valores (Dur, Int, Cmd, Modo, ProfileId, ActionType)
            numParsed = sscanf(rxValue.c_str(), "%u,%u,%u,%u,%u,%u", 
                                &durationTemp, &intensityTemp, &commandTemp, 
                                &modeTemp, &profileIdTemp, &actionTypeTemp);
            
            if (numParsed == 6) { // Verifica se todos os 6 campos foram lidos
                EmdrCommandFromApp_t incomingCommand;
                
                // Mapeamento para a nova estrutura
                incomingCommand.durationPerSideMs = durationTemp;
                incomingCommand.intensityPercent = intensityTemp;
                incomingCommand.command = commandTemp;
                incomingCommand.actuatorMode = modeTemp;
                incomingCommand.profileId = profileIdTemp;
                incomingCommand.actionType = actionTypeTemp;

                // Passa o comando completo (incluindo SAVE/LOAD) para a lógica central
                handleCommand(incomingCommand); 
            } else {
                Serial.printf("[HUB]: ERRO de comando BLE. Recebido %d/%d campos. Conteudo: %s\n", numParsed, 6, rxValue.c_str());
            }
        }
    }
};

// 3. CALLBACKS DE LEITURA (ALL_PROFILES)
class AllProfilesCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) {
        Serial.println("[HUB]: Recebida requisicao de READ para ALL_PROFILES. Carregando 35 bytes...");
        
        // 1. Cria a estrutura para armazenar os 5 perfis
        EmdrAllProfiles_t allProfilesData;
        
        // 2. Chama a funcao do profile_manager para carregar os 5 slots
        loadAllProfiles(&allProfilesData);
        
        // 3. Define o valor da característica com os 35 bytes brutos (para o App ler)
        pCharacteristic->setValue((uint8_t*)&allProfilesData, sizeof(EmdrAllProfiles_t));
        
        Serial.printf("[HUB]: %d bytes de todos os perfis enviados.\n", sizeof(EmdrAllProfiles_t));
    }
};


// --- Funções de Inicialização ---

void init_ble_server() {
    BLEDevice::init(DEVICE_NAME_FIXO);
    pServer = BLEDevice::createServer(); // Usa o pServer global
    pServer->setCallbacks(new MyServerCallbacks());

    // 1. SERVIÇO PRINCIPAL (APP <-> PRINCIPAL)
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Caracteristica de Status/Notificacao (7 bytes - Config Ativa)
    pStatusCharacteristic = pService->createCharacteristic( 
        STATUS_CHAR_UUID, 
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY 
    );
    pStatusCharacteristic->addDescriptor(new BLE2902()); 
    
    // Caracteristica de Comando/Escrita (9 bytes - Comando App)
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
    loadAllProfiles(&initialProfiles); // Usa a função do profile_manager.cpp
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
    pAdvertising->addServiceUUID(SERVICE_UUID); // Anuncia o Serviço App
    BLEDevice::startAdvertising();
    Serial.println("--- BLE Server Inicializado e Anunciando. ---");
}