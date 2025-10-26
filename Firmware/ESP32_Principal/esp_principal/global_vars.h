// Arquivo: global_vars.h (Versão Final e Completa)

#pragma once

#include <Arduino.h>
#include <stdint.h>             // <<< ADICIONADO: Para uint8_t, uint32_t
#include <stddef.h>             // <<< ADICIONADO: Para size_t
#include <BLEServer.h>
#include <BLECharacteristic.h>  // <<< ADICIONADO: Para BLECharacteristic* (boa prática)
#include "Adafruit_DRV2605.h" 
#include "data_struct.h" 


// --- VARIÁVEIS BLE ---
extern BLEServer* pServer;
extern BLECharacteristic* pCommandCharacteristic;
extern BLECharacteristic* pStatusCharacteristic;
extern BLECharacteristic* pAuxCommandCharacteristic;
extern BLECharacteristic* pAllProfilesCharacteristic; 

extern bool deviceConnected;
extern bool oldDeviceConnected;
extern uint32_t value; 

// --- VARIÁVEIS DE CONTROLE DE CICLO E CONFIG ---
extern EmdrConfigData_t currentConfig;
extern bool isRunning;
extern unsigned long pulseDuration;
extern unsigned long lastToggleTime;
extern bool isLocalActive; 

// --- VARIÁVEIS DE HARDWARE E CONSTANTES ---
extern const uint8_t DRV_ADDR_M2;
extern Adafruit_DRV2605 drv_m2;
extern const int LED_PIN;
extern const size_t CONFIG_DATA_SIZE;