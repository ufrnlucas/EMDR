// Arquivo: ESP32_Auxiliar/ESP32_Auxiliar.ino (Conexao e Status Simples - ASCII Puro)

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <cstring> 

// --- DEFINICOES DE COMUNICACAO (Devem ser iguais as do Principal) ---
#define DEVICE_NAME_TARGET  "ESP32" 
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define STATUS_CHAR_UUID    "19b10001-e8f2-537e-4f6c-d104768a1214"

// --- VARIAVEIS GLOBAIS DE CONEXAO ---
static BLEAddress serverAddress(""); 
static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* pPrincipalAdvertisedDevice = nullptr; 
static BLEClient* pClient = nullptr;

// Pino de LED para feedback visual de CONEXAO
const int LED_STATUS_PIN = 2; 

// =========================================================
// 1. LOGICA DE CONEXAO E DESCONEXAO
// =========================================================

// Classe de Callback de Desconexao (Limpa o estado se o Principal cair)
class MyClientCallbacks : public BLEClientCallbacks {
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    digitalWrite(LED_STATUS_PIN, LOW);
    Serial.println("\n--- AUX: DESCONECTADO do Servidor. Reiniciando scanner... ---");
    // Se desconectou, precisamos escanear novamente
    doConnect = false;
    BLEDevice::getScan()->start(5, false);
  }
};

bool connectToServer() {
    Serial.print("--- AUX: Tentando conectar a ");
    Serial.println(serverAddress.toString().c_str());

    // 1. Cria e Conecta o Cliente
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks());

    if (!pClient->connect(serverAddress)) {
        Serial.println("--- AUX: FALHA na conexao inicial.");
        return false;
    }
    Serial.println("--- AUX: CONECTADO ao servidor HUB!");

    // 2. Verifica o Servico
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == nullptr) {
        Serial.println("--- AUX: ERRO: Servico principal nao encontrado!");
        pClient->disconnect();
        return false;
    }

    // 3. Verifica a Caracteristica de Status
    BLERemoteCharacteristic* pRemoteStatusCharacteristic = pRemoteService->getCharacteristic(STATUS_CHAR_UUID);
    if (pRemoteStatusCharacteristic == nullptr) {
        Serial.println("--- AUX: ERRO: Caracteristica de Status nao encontrada!");
        pClient->disconnect();
        return false;
    }

    // Sucesso Total
    connected = true;
    digitalWrite(LED_STATUS_PIN, HIGH);
    Serial.println("--- AUX: Conexao e Servicos verificados com sucesso.");
    return true;
}

// =========================================================
// 2. LOGICA DE SCAN
// =========================================================

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.getName() == DEVICE_NAME_TARGET) {
            Serial.println("--- AUX: Dispositivo HUB encontrado!");
            serverAddress = advertisedDevice.getAddress();
            
            // Para o scan e define a flag de conexao
            advertisedDevice.getScan()->stop();
            doConnect = true;
        }
    }
};

// =========================================================
// 3. SETUP E LOOP
// =========================================================

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando ESP32 Auxiliar (Cliente Simples)...");
    
    pinMode(LED_STATUS_PIN, OUTPUT);
    digitalWrite(LED_STATUS_PIN, LOW);
    
    BLEDevice::init("");

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false); // Scan de 5 segundos
}

void loop() {
    // 3.1. Tenta Conectar se o Scan encontrou o Principal
    if (doConnect) {
        doConnect = false;
        if (connectToServer()) {
            // Fica conectado, esperando a desconexão ou fim do programa
        }
    }

    // 3.2. Feedback Visual e Scan de Reconexao
    if (!connected) {
        // Se estiver desconectado, pisca o LED lentamente
        digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN));
        delay(500); 
    } else {
        // Se estiver conectado, o loop fica ocioso
        delay(10); 
    }
}