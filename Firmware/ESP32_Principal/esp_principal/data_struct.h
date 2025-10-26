// Arquivo: data_struct.h (Tipagem Unificada)

#pragma once

#include <stdint.h>
#include <stddef.h> 

// CRÍTICO: Usar unsigned long para garantir compatibilidade com %lu no sscanf
typedef struct __attribute__((packed)) {
    unsigned long durationPerSideMs; // 4 bytes
    uint8_t intensityPercent;
    uint8_t command; 
    uint8_t actuatorMode;
} EmdrConfigData_t; // Total: 7 bytes


// --- NOVO: ESTRUTURA PARA TRANSFERÊNCIA DE TODOS OS PERFIS ---
typedef struct __attribute__((packed)) {
    EmdrConfigData_t profiles[5];
} EmdrAllProfiles_t;


// CRÍTICO: Usar unsigned long para corresponder ao tipo de destino do sscanf (%lu)
typedef struct __attribute__((packed)) {
    unsigned long durationPerSideMs; // << CORREÇÃO AQUI
    uint8_t intensityPercent;
    uint8_t command;
    uint8_t actuatorMode;
    uint8_t profileId;
    uint8_t actionType;
} EmdrCommandFromApp_t;