// Arquivo: emdr_config.h

#pragma once

#include <Arduino.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include "Adafruit_DRV2605.h" 

// --- Estruturas de Dados ---
// Assumimos que esta struct existe no projeto (data_struct.h ou aqui)
struct EmdrConfigData_t {
  uint32_t durationPerSideMs;
  uint8_t intensityPercent;
  uint8_t command;
  uint8_t actuatorMode;
};

// --- UUIDs de Comunicação ---
#define DEVICE_NAME_FIXO    "ESP32" 
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define COMMAND_CHAR_UUID   "19b10002-e8f2-537e-4f6c-d104768a1214"
#define STATUS_CHAR_UUID    "19b10001-e8f2-537e-4f6c-d104768a1214"

// --- Variáveis Globais do Projeto ---
extern BLEServer* pServer;
extern BLECharacteristic* pCommandCharacteristic;
extern BLECharacteristic* pStatusCharacteristic;
extern bool deviceConnected;
extern bool oldDeviceConnected;
extern uint32_t value; // Contador para notificação de status

// --- Variáveis de Controle de Ciclo ---
extern EmdrConfigData_t currentConfig;
extern bool isRunning;
extern unsigned long pulseDuration;
extern unsigned long lastToggleTime;
extern bool isLocalActive; 
extern const size_t CONFIG_DATA_SIZE; // Adicionada aqui para evitar erros de escopo

// --- Variáveis de Atuadores ---
extern const uint8_t DRV_ADDR_M2;
extern Adafruit_DRV2605 drv_m2;
extern const int LED_PIN;