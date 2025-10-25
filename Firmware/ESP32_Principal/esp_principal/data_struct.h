// Arquivo: data_struct.h (CORREÇÃO FINAL DE PACKING COM PRAGMA)
// Arquivo: data_struct.h (Modificação Crítica)
#pragma once

typedef struct {
    // MUDANÇA: uint16_t (2 bytes) para uint32_t (4 bytes)
    uint32_t durationPerSideMs; 
    uint8_t intensityPercent;
    uint8_t command;
} EmdrConfigData_t;

// CRÍTICO: O tamanho da struct MUDOU de 4 para 6 bytes (4 + 1 + 1)
const size_t CONFIG_DATA_SIZE = 6;