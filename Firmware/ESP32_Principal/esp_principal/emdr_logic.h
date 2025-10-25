// Arquivo: emdr_logic.h

#pragma once
#include "emdr_config.h"
#include "motor_control.h"

// Funções expostas
void updateStatusCharacteristic();
void stopStimulation();
void handleCommand(EmdrConfigData_t config);
void run_emdr_cycle(); // Chamada no loop()