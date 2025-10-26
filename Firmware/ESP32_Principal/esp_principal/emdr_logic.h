// Arquivo: emdr_logic.h (Versão Limpa)

#pragma once

#include "global_vars.h"     // <<< NOVO: Para acessar currentConfig, pStatusCharacteristic, etc.
#include "motor_control.h"   // Para stopStimulation e outras chamadas de motor
#include "profile_manager.h" // Para a struct EmdrCommandFromApp_t

// Funções expostas
void updateStatusCharacteristic();
void stopStimulation();
void handleCommand(EmdrCommandFromApp_t command);
void run_emdr_cycle(); // Chamada no loop()