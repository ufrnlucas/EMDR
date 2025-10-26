// Arquivo: data_struct.h

#pragma once

#include <stdint.h>
#include <stddef.h> 

// (MANTENHA A ESTRUTURA EmdrConfigData_t INTACTA)
typedef struct __attribute__((packed)) {
    uint32_t durationPerSideMs; // 4 bytes
    uint8_t intensityPercent;   // 1 byte (Mapeado 0-100 para 5-70)
    uint8_t command;            // 1 byte (1=START, 2=STOP, 3=PAUSE, 4=RESTART)
    uint8_t actuatorMode;       // 1 byte (1=Motor, 2=LED, 3=Ambos)
} EmdrConfigData_t; // Total: 7 bytes


// --- NOVO: ESTRUTURA PARA TRANSFERÊNCIA DE TODOS OS PERFIS ---
// Contém os 5 perfis para leitura em bloco pelo BLE (5 x 7 = 35 bytes)
typedef struct __attribute__((packed)) {
    EmdrConfigData_t profiles[5];
} EmdrAllProfiles_t;


// (MANTENHA A ESTRUTURA EmdrCommandFromApp_t INTACTA)
typedef struct __attribute__((packed)) {
    uint32_t durationPerSideMs; 
    uint8_t intensityPercent;   
    uint8_t command;            
    uint8_t actuatorMode;       
    uint8_t profileId;          // Slot de perfil (1-5)
    uint8_t actionType;         // 1=RUN/LOAD, 2=SAVE
} EmdrCommandFromApp_t;