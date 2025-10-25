// Arquivo: data_struct.h

#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <stddef.h>
#include <stdint.h>

// Utilizamos __attribute__((packed)) e colocamos os campos de 1 byte primeiro
// para garantir o alinhamento de 4 bytes [uint8, uint8, uint16].
typedef struct __attribute__((packed)) { 
  
  // 1. Campos de 1 byte
  uint8_t intensityPercent;   // 1 byte
  uint8_t command;            // 1 byte

  // 2. Campo de 2 bytes (último)
  uint16_t durationPerSideMs; // 2 bytes
  
} EmdrConfigData_t;

const size_t CONFIG_DATA_SIZE = 4; // 1 + 1 + 2 = 4 bytes

#endif // DATA_STRUCT_H