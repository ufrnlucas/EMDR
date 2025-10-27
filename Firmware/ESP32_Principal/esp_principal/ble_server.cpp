#include "ble_server.h"
#include "global_vars.h"
#include "profile_manager.h" 
#include "emdr_logic.h"
#include <BLEDevice.h>
#include <BLEUtils.h>

// --- UUIDs do Serviço e Características ---
// Usando as constantes definidas, por favor, certifique-se de que ble_uuids.h
// foi incluído ou que você as definiu globalmente. Reutilizando os valores do seu contexto.
#define SERVICE_UUID "19b10000-e8f2-537e-4f6c-d104768a1214"
#define STATUS_CHAR_UUID "19b10001-e8f2-537e-4f6c-d104768a1214" 
#define COMMAND_CHAR_UUID  "19b10002-e8f2-537e-4f6c-d104768a1214" 
#define AUX_COMMAND_CHAR_UUID "19b10004-e8f2-537e-4f6c-d104768a1214" 
#define ALL_PROFILES_CHAR_UUID "19b10005-e8f2-537e-4f6c-d104768a1214" 

// --- Variáveis Globais (Definidas em global_vars.cpp) ---

// --- 1. FUNÇÃO DE DEBUG HEX (para o Serial Monitor) ---
void debugPrintHex(const uint8_t* buffer, size_t length) {
  Serial.print("[BLE_HEX_OUT]: ");
  Serial.print(length);
  Serial.print(" bytes = ");
  for (size_t i = 0; i < length; i++) {
    if (buffer[i] < 0x10) {
      Serial.print("0"); 
    }
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// --- 2. CALLBACKS DE CONEXÃO ---
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("[HUB]: Novo cliente conectado. Total: 0");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("[HUB]: Cliente desconectado.");
        stopStimulation(); // Para a vibração ao desconectar
    }
};

// --- 3. CALLBACKS DE ESCRITA (COMMAND) ---
class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        // CORREÇÃO: Cria explicitamente um std::string para resolver o conflito de tipos.
        std::string rxValue(pCharacteristic->getValue().c_str()); 

        if (rxValue.length() > 0) {
            // Assume que o valor é uma string CSV: duration, intensity, command, actuatorMode, profileId, actionType
            Serial.printf("[DEBUG PARSE]: Conteudo lido: %s\n", rxValue.c_str());

            char* str = (char*)rxValue.c_str();
            
            // Variáveis temporárias para o comando
            EmdrCommandFromApp_t command;

            // Formato esperado: %lu,%hhu,%hhu,%hhu,%hhu,%hhu
            // long, uchar, uchar, uchar, uchar, uchar
            int numParsed = sscanf(str, "%lu,%hhu,%hhu,%hhu,%hhu,%hhu",
                &command.durationPerSideMs, 
                &command.intensityPercent,
                &command.command,
                &command.actuatorMode,
                &command.profileId,
                &command.actionType
            );

            Serial.printf("[DEBUG PARSE]: numParsed: %d, Duracao: %lu\n", numParsed, command.durationPerSideMs);

            if (numParsed == 6) {
                // A lógica principal é delegada a emdr_logic.cpp
                handleCommand(command);
            } else {
                Serial.println("ERRO PARSE: Formato de comando invalido. Ignorando.");
            }
        }
    }
};

// --- 4. CALLBACKS DE LEITURA (ALL_PROFILES) ---
class AllProfilesCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) {
        
        Serial.println(">>> DEBUG TESTE: CALLBACK ONREAD INICIADO <<<"); // TESTE DE EXECUÇÃO

        Serial.println("[HUB]: Recebida requisicao de READ para ALL_PROFILES. Carregando 35 bytes...");
        
        EmdrAllProfiles_t allProfilesData;
        loadAllProfiles(&allProfilesData);
        
        // DEBUG CRÍTICO - Imprime o buffer exato antes de enviar
        debugPrintHex((uint8_t*)&allProfilesData, sizeof(EmdrAllProfiles_t));

        pCharacteristic->setValue((uint8_t*)&allProfilesData, sizeof(EmdrAllProfiles_t));
        
        Serial.printf("[HUB]: %d bytes de todos os perfis enviados.\n", sizeof(EmdrAllProfiles_t));
    }
};

// --- 5. INICIALIZAÇÃO DO SERVIDOR BLE ---
void init_ble_server() {
    Serial.println("[BLE]: Inicializando Servidor BLE...");

    BLEDevice::init("ESP32"); 

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService* pService = pServer->createService(SERVICE_UUID);

    // 1. Característica STATUS (Read/Notify - 7 bytes)
    pStatusCharacteristic = pService->createCharacteristic(
        STATUS_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    // 2. Característica COMMAND (Write - String)
    pCommandCharacteristic = pService->createCharacteristic(
        COMMAND_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());


    // 3. Característica AUX_COMMAND (Write/Notify - 1 byte)
    pAuxCommandCharacteristic = pService->createCharacteristic(
        AUX_COMMAND_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );

    // 4. Característica ALL_PROFILES (Read - 35 bytes)
    pAllProfilesCharacteristic = pService->createCharacteristic(
        ALL_PROFILES_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    pAllProfilesCharacteristic->setCallbacks(new AllProfilesCharacteristicCallbacks());


    // --- Inicialização do Caching (CRÍTICO) ---
    // CORREÇÃO: Força o cache da característica ALL_PROFILES a ser o conteúdo atual da EEPROM.
    EmdrAllProfiles_t initialProfiles;
    loadAllProfiles(&initialProfiles); 
    pAllProfilesCharacteristic->setValue((uint8_t*)&initialProfiles, sizeof(EmdrAllProfiles_t));
    
    // Inicia o serviço
    pService->start();

    // Inicia a publicidade (Advertising)
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Tamanho máximo de dados
    pAdvertising->start();

    Serial.println("[BLE]: Servidor e Advertising iniciados. ESP32 pronto para conexão.");
}