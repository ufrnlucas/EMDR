// Arquivo: firmware/common/data_struct.h

#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <Arduino.h>

typedef struct {
    // Duração que CADA motor deve vibrar (em milissegundos) (2 bytes)
    uint16_t durationPerSideMs; 

    // Intensidade da vibração (0 a 100, para mapear para o ciclo de trabalho PWM) (1 byte)
    uint8_t intensityPercent;     
    
    // Comando de Ação: 1 = Iniciar, 0 = Parar (1 byte)
    uint8_t command;     

} EmdrConfigData_t; 

#define CONFIG_DATA_SIZE sizeof(EmdrConfigData_t)

#endif