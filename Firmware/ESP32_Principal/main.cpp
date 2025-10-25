// Arquivo: firmware/ESP32_PRINCIPAL/ESP32_PRINCIPAL.ino

// Incluindo arquivos de cabeçalho (Deve estar na pasta ../common/)
#include "../common/ble_uuids.h" 
#include "../common/data_struct.h" 

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>

// =========================================================
// 1. VARIÁVEIS GLOBAIS E DEFINIÇÕES DE HARDWARE
// =========================================================

// Pino do LED nativo do ESP32-C3 Super Mini (Geralmente GPIO 8 ou LED_BUILTIN)
// *VERIFIQUE O SEU PINO SE NÃO ACENDER!*
const int LED_PIN = 8; 

// Variáveis do Servidor BLE (Comunicação com o Site)
BLEServer* pServer = NULL;
BLECharacteristic* pCommandCharacteristic = NULL;
BLECharacteristic* pStatusCharacteristic = NULL; 
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Variável para armazenar a última configuração de comando recebida (4 bytes)
EmdrConfigData_t latestConfig; 
bool isRunning = false; // Estado de controle do ciclo de EMDR

// =========================================================
// 2. LÓGICA DE GATEWAY (Validação do Comando)
// =========================================================

// Função de tratamento do comando e validação do LED
void handleCommand(EmdrConfigData_t config) {
  
  Serial.println("=============================================");
  Serial.println(">>> DADOS RECEBIDOS E DECODIFICADOS:");
  Serial.printf("Duração por lado: %u ms\n", config.durationPerSideMs);
  Serial.printf("Intensidade: %u %%\n", config.intensityPercent);
  
  // Lógica de Início
  if (config.command == 1) {
    Serial.println("COMANDO: INICIAR VIBRAÇÃO.");
    // Validação de Início: Liga o LED (HIGH)
    digitalWrite(LED_PIN, HIGH);
    isRunning = true;
    
    // Armazena a configuração
    latestConfig = config; 
    
  // Lógica de Parada
  } else if (config.command == 0) {
    Serial.println("COMANDO: PARAR VIBRAÇÃO.");
    // Validação de Parada: Desliga o LED (LOW)
    digitalWrite(LED_PIN, LOW);
    isRunning = false;
  }
  Serial.println("=============================================");
}

// =========================================================
// 3. CALLBACKS DO SERVIDOR BLE (Recebimento de Comandos do Site)
// =========================================================

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Cliente (Site Web BLE) conectado.");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Cliente (Site Web BLE) desconectado.");
    }
};

class CommandCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // Recebe o buffer binário (4 bytes)
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() == CONFIG_DATA_SIZE) {
        EmdrConfigData_t incomingConfig; 
        // Mapeia os bytes recebidos para a struct
        memcpy(&incomingConfig, rxValue.c_str(), CONFIG_DATA_SIZE);
        
        // Chama a função de tratamento e validação
        handleCommand(incomingConfig);

      } else {
        Serial.printf("Erro: Recebido %d bytes, esperado %d bytes.\n", 
                      rxValue.length(), CONFIG_DATA_SIZE);
      }
    }
};

// =========================================================
// 4. CONFIGURAÇÃO (SETUP)
// =========================================================

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32 Principal (Gateway)");
  
  // Configuração do LED nativo para validação
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 
  
  // 4.1. Configuração do SERVIDOR BLE (Para o Site)
  BLEDevice::init(DEVICE_NAME_PRINCIPAL); // Define o nome anunciado
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Cria o Serviço
  BLEService *pService = pServer->createService(SERVICE_UUID_EXT);

  // Cria a Characteristic de Comando (WRITE/WRITE_NR)
  pCommandCharacteristic = pService->createCharacteristic(
                            CHAR_UUID_COMMAND_EXT,
                            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
                          );
  pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
  pCommandCharacteristic->addDescriptor(new BLE2902()); 

  // Cria a Characteristic de Status (READ/NOTIFY) - Opcional para Feedback
  pStatusCharacteristic = pService->createCharacteristic(
                            CHAR_UUID_STATUS_EXT,
                            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                          );
  pStatusCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  
  // 4.2. Inicia a Publicidade (Advertising)
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID_EXT);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  BLEDevice::startAdvertising();
  Serial.println("Servidor BLE pronto e anunciando como 'EMDR_PRINCIPAL'...");
}

// =========================================================
// 5. LOOP PRINCIPAL
// =========================================================

void loop() {
  // Lógica de reconexão do Site (se desconectar)
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); 
      pServer->startAdvertising(); 
      Serial.println("Reiniciando o anúncio...");
      oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
      // Ações quando o cliente se conecta
      oldDeviceConnected = deviceConnected;
  }
  
  // *A Lógica de Cliente BLE e Retransmissão será adicionada aqui*

  delay(100);
}