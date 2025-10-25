// Arquivo: data_struct.h (CORREÇÃO FINAL DE PACKING COM PRAGMA)

#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <stddef.h>
#include <stdint.h>


// A ordem é a mesma do JavaScript: [uint16 Duração] | [uint8 Intensidade] | [uint8 Comando]
typedef struct { 
  uint16_t durationPerSideMs; // 2 bytes
  uint8_t intensityPercent;   // 1 byte
  uint8_t command;            // 1 byte
} EmdrConfigData_t;

// CRÍTICO: Restaura o alinhamento padrão
#pragma pack(pop) 

// O tamanho real deve ser 4
const size_t CONFIG_DATA_SIZE = 4;

#endif // DATA_STRUCT_H