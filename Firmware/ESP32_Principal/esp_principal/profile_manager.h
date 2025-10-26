// Arquivo: profile_manager.h (Versão Corrigida e Completa)

#pragma once
#include "data_struct.h"

// --- Constantes para EEPROM ---
const int CONFIG_SLOT_SIZE = sizeof(EmdrConfigData_t); // 7 bytes
const int NUM_PROFILE_SLOTS = 5;
const int EEPROM_TOTAL_SIZE = CONFIG_SLOT_SIZE * NUM_PROFILE_SLOTS; // 35 bytes
const int EEPROM_START_ADDR = 0;

// Funções para gerenciar a memória
void init_profile_manager();
void save_config_to_slot(uint8_t slotId, const EmdrConfigData_t& config);
bool load_config_from_slot(uint8_t slotId, EmdrConfigData_t& config);

// NOVO: Funções para carregar todos os perfis (solução de 35 bytes)
void loadAllProfiles(EmdrAllProfiles_t* allProfiles);