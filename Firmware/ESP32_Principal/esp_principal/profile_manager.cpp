// Arquivo: profile_manager.cpp

#include "profile_manager.h"
#include <EEPROM.h>

// Calcula o endereço de memória para o slot de 1 a 5.
// Note: O codigo usa slotId (1..5), entao subtraimos 1 para o indice do array (0..4).
int get_eeprom_addr(uint8_t slotId) {
    if (slotId < 1 || slotId > NUM_PROFILE_SLOTS) return -1;
    return EEPROM_START_ADDR + (slotId - 1) * CONFIG_SLOT_SIZE;
}

void init_profile_manager() {
    // Apenas aloca e inicializa o tamanho da EEPROM virtual
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

    // Escreve a struct (7 bytes) byte a byte no endereco
    EEPROM.put(addr, config);
    
    // Confirma a escrita no Flash
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
    
    // Le a struct (7 bytes) do endereco e a copia para a variavel 'config'
    EEPROM.get(addr, config);
    
    // Para validacao: Se a duracao for 0 ou 0xFFFFFFFF, assumimos que o slot esta vazio/nao inicializado
    if (config.durationPerSideMs == 0 || config.durationPerSideMs == 0xFFFFFFFF) {
        Serial.printf("[EEPROM]: Perfil %u (endereco %d) Vazio ou Invalido.\n", slotId, addr);
        return false;
    }
    
    Serial.printf("[EEPROM]: Perfil %u carregado com sucesso. Duracao: %lu\n", slotId, config.durationPerSideMs);
    return true;
}

// Carrega todos os 5 perfis para a estrutura EmdrAllProfiles_t (35 bytes)
void loadAllProfiles(EmdrAllProfiles_t* allProfiles) {
    
    // Itera pelos 5 slots (o índice do array 'i' é de 0 a 4)
    for (uint8_t i = 0; i < NUM_PROFILE_SLOTS; i++) {
        uint8_t slotId = i + 1; // Slot ID é de 1 a 5
        
        // Carrega o perfil do slot 'slotId' e armazena diretamente no array da struct
        int addr = get_eeprom_addr(slotId);
        
        if (addr != -1) {
             // Usa EEPROM.get() para ler 7 bytes diretamente para a posição i do array
             EEPROM.get(addr, allProfiles->profiles[i]);
             
             // Nota: Não é estritamente necessário verificar se está vazio aqui,
             // mas é bom saber. A validação real de 'vazio' será feita pelo site.
             if (allProfiles->profiles[i].durationPerSideMs == 0 || allProfiles->profiles[i].durationPerSideMs == 0xFFFFFFFF) {
                // Se estiver vazio, define um valor seguro (ex: 0, 0, 0, 0) para o App
                allProfiles->profiles[i] = {0, 0, 0, 0}; 
             }
             
        } else {
             // Se o slot for inválido por algum motivo (embora não deva acontecer)
             allProfiles->profiles[i] = {0, 0, 0, 0}; 
        }
    }
    
    Serial.printf("[EEPROM]: Carregamento em bloco de %d perfis completo.\n", NUM_PROFILE_SLOTS);
}