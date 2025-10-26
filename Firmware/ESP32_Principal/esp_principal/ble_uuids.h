// Arquivo: firmware/common/ble_uuids.h (Revisado)

#ifndef BLE_UUIDS_H
#define BLE_UUIDS_H

// Arquivo: ble_uuids.h (Definições de Constantes Necessárias)

// UUIDs para Comunicação App <-> Principal
#define DEVICE_NAME_FIXO "ESP32"
#define SERVICE_UUID "19b10000-e8f2-537e-4f6c-d104768a1214"
#define STATUS_CHAR_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"
#define COMMAND_CHAR_UUID "19b10002-e8f2-537e-4f6c-d104768a1214"

// NOVO: Característica para leitura de todos os 5 perfis (35 bytes)
#define ALL_PROFILES_CHAR_UUID "19b10005-e8f2-537e-4f6c-d104768a1214" 


// Constantes para Comunicação Interna (Principal <-> Auxiliar)
#define DEVICE_NAME_ESCRAVO "ESP_AUXILIAR" 
#define SERVICE_UUID_INT "19b10003-e8f2-537e-4f6c-d104768a1214" // Reservado
#define CHAR_UUID_SYNC_INT "19b10004-e8f2-537e-4f6c-d104768a1214" // Reservado

#endif