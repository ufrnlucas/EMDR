// Arquivo: profile_manager.cpp

#include "profile_manager.h"
#include <EEPROM.h>

// Calcula o endereço de memória para o slot de 1 a 5.
int get_eeprom_addr(uint8_t slotId) {
    if (slotId < 1 || slotId > NUM_PROFILE_SLOTS) return -1;
    return EEPROM_START_ADDR + (slotId - 1) * CONFIG_SLOT_SIZE;
}

void init_profile_manager() {
    if (!EEPROM.begin(EEPROM_TOTAL_SIZE)) {
        Serial.println("ERRO FATAL: Falha ao inicializar EEPROM.");
    } else {
        Serial.printf("[EEPROM]: Memoria para %d perfis (%d bytes) inicializada.\n", NUM_PROFILE_SLOTS, EEPROM_TOTAL_SIZE);
    }
}

// Salva a configuracao no slot especificado (1 a 5)
void save_config_to_slot(uint8_t slotId, const EmdrConfigData_t& config) {
    int addr = get_eeprom_addr(slotId);
    if (addr == -1) {
        Serial.println("ERRO: Tentativa de salvar em slot invalido.");
        return;
    }

    // DEBUG: O que estamos prestes a salvar
    Serial.printf("[DEBUG SAVE]: Slot %u. Duracao ENVIADA: %lu. Endereco: %d\n", 
                  slotId, config.durationPerSideMs, addr); 

    EEPROM.put(addr, config);
    
    if (EEPROM.commit()) {
        Serial.printf("[EEPROM]: Perfil %u salvo com sucesso no endereco %d.\n", slotId, addr);
    } else {
        Serial.println("ERRO: Falha ao salvar EEPROM commit.");
    }
}

// Carrega a configuracao do slot especificado (1 a 5)
bool load_config_from_slot(uint8_t slotId, EmdrConfigData_t& config) {
    int addr = get_eeprom_addr(slotId);
    if (addr == -1) {
        Serial.println("ERRO: Tentativa de carregar de slot invalido.");
        return false;
    }
    
    EEPROM.get(addr, config);
    
    // DEBUG: O que acabamos de ler
    Serial.printf("[DEBUG LOAD]: Slot %u. Duracao LIDA: %lu. Endereco: %d\n", 
                  slotId, config.durationPerSideMs, addr); 
    
    if (config.durationPerSideMs == 0 || config.durationPerSideMs == 0xFFFFFFFF) {
        Serial.printf("[EEPROM]: Perfil %u (endereco %d) Vazio ou Invalido.\n", slotId, addr);
        return false;
    }
    
    Serial.printf("[EEPROM]: Perfil %u carregado com sucesso. Duracao: %lu\n", slotId, config.durationPerSideMs);
    return true;
}

// Carrega todos os 5 perfis para a estrutura EmdrAllProfiles_t (35 bytes)
void loadAllProfiles(EmdrAllProfiles_t* allProfiles) {
    
    for (uint8_t i = 0; i < NUM_PROFILE_SLOTS; i++) {
        uint8_t slotId = i + 1;
        int addr = get_eeprom_addr(slotId);
        
        if (addr != -1) {
             EEPROM.get(addr, allProfiles->profiles[i]);
             
             if (allProfiles->profiles[i].durationPerSideMs == 0 || allProfiles->profiles[i].durationPerSideMs == 0xFFFFFFFF) {
                 allProfiles->profiles[i] = {0, 0, 0, 0}; 
             }
             
        } else {
             allProfiles->profiles[i] = {0, 0, 0, 0}; 
        }
    }
    
    Serial.printf("[EEPROM]: Carregamento em bloco de %d perfis completo.\n", NUM_PROFILE_SLOTS);
}