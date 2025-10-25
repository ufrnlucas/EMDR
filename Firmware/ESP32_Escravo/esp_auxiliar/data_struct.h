// Arquivo: data_struct.h (Com Modo de Atuador)
#pragma once

typedef struct {
    uint32_t durationPerSideMs; 
    uint8_t intensityPercent;
    uint8_t command;
    // NOVO CAMPO: 1 byte para o modo de atuação
    uint8_t actuatorMode; // 1=Motor, 2=LED, 3=Ambos
} EmdrConfigData_t;

// CRÍTICO: O tamanho da struct MUDOU novamente (4 + 1 + 1 + 1)
const size_t CONFIG_DATA_SIZE = 7;