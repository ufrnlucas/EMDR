// Arquivo: global_vars.cpp

#include "global_vars.h"

// --- VARIÁVEIS BLE ---
BLEServer* pServer = nullptr;
BLECharacteristic* pCommandCharacteristic = nullptr;
BLECharacteristic* pStatusCharacteristic = nullptr;
BLECharacteristic* pAuxCommandCharacteristic = nullptr; 
BLECharacteristic* pAllProfilesCharacteristic = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0; 

// --- VARIÁVEIS DE CONTROLE DE CICLO E CONFIG ---
EmdrConfigData_t currentConfig = {1000, 100, 0, 3}; // Inicialização padrão
bool isRunning = false;
unsigned long pulseDuration = 0;
unsigned long lastToggleTime = 0;
bool isLocalActive = false; 

// --- VARIÁVEIS DE HARDWARE E CONSTANTES ---
const uint8_t DRV_ADDR_M2 = 0x5A; 
Adafruit_DRV2605 drv_m2;
const int LED_PIN = 1; 
const size_t CONFIG_DATA_SIZE = 7;