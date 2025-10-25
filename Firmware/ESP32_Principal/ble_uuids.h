// Arquivo: firmware/common/ble_uuids.h

#ifndef BLE_UUIDS_H
#define BLE_UUIDS_H

// =========================================================
// 1. COMUNICAÇÃO EXTERNA: SITE (CLIENTE) <-> ESP32_PRINCIPAL (SERVIDOR)
// USADAS PELO SITE
// =========================================================
// SERVICE: O contêiner para a comunicação com o site.
#define SERVICE_UUID_EXT "19b10000-e8f2-537e-4f6c-d104768a1214"

// CHARACTERISTIC DE COMANDO: Recebe o comando de 4 bytes (Duração, Intensidade, Comando)
#define CHAR_UUID_COMMAND_EXT "19b10002-e8f2-537e-4f6c-d104768a1214" 

// CHARACTERISTIC DE STATUS: Opcional, para o ESP32 enviar feedback ao Site (NOTIFY)
#define CHAR_UUID_STATUS_EXT "19b10001-e8f2-537e-4f6c-d104768a1214" 

// Nome que o ESP32 Principal irá anunciar no BLE
#define DEVICE_NAME_PRINCIPAL "EMDR_PRINCIPAL"

// =========================================================
// 2. COMUNICAÇÃO INTERNA: ESP32_PRINCIPAL (CLIENTE) <-> ESP32_ESCRAVO (SERVIDOR)
// USADAS PELO ESP32_PRINCIPAL e ESP32_ESCRAVO
// =========================================================

// Nome que o ESP32 Escravo irá anunciar no BLE
#define DEVICE_NAME_ESCRAVO "EMDR_ESCRAVO" 

// SERVICE M2M: Service Exclusivo para Comunicação Máquina-a-Máquina
#define SERVICE_UUID_INT "4fafc201-1fb5-459e-8fcc-c5c9c331914b" 

// CHARACTERISTIC DE SINCRONIZAÇÃO: O ESP32 Principal escreve nela para iniciar a vibração síncrona.
#define CHAR_UUID_SYNC_INT "beb5483e-36e1-4688-b7f5-ea07361b26a8" 

#endif