// Arquivo: emdr_logic.cpp (Versão Limpa e Funcional)

#include "emdr_logic.h"
#include "global_vars.h" // Necessário para acessar todas as variáveis globais
#include "profile_manager.h" // Necessário para salvar/carregar perfis

// --- Funções de Lógica e Controle ---

void updateStatusCharacteristic() {
    // Note: pStatusCharacteristic está agora em global_vars
    if (pStatusCharacteristic) {
        // Envia a configuracao atual (7 bytes)
        pStatusCharacteristic->setValue((uint8_t*)&currentConfig, CONFIG_DATA_SIZE);
    }
}

void stopStimulation() {
    isRunning = false;
    setMotorIntensity(drv_m2, 0); // Usa variáveis de hardware de global_vars
    digitalWrite(LED_PIN, LOW); // Usa variáveis de hardware de global_vars
    Serial.println("-> [LOGIC]: Estimulacao PARADA.");

    // NOVO: Garantir que o Auxiliar também pare (enviando 0)
    if (pAuxCommandCharacteristic) {
        uint8_t aux_cmd = 0; // 0 = DESLIGA
        pAuxCommandCharacteristic->setValue(&aux_cmd, 1);
        pAuxCommandCharacteristic->notify();
    }
}

void controlActuatorsLocal(bool activate) {
    bool controlMotor = (currentConfig.actuatorMode == 1 || currentConfig.actuatorMode == 3);
    bool controlLED = (currentConfig.actuatorMode == 2 || currentConfig.actuatorMode == 3);
    
    if (activate) {
        if (controlMotor) setMotorIntensity(drv_m2, currentConfig.intensityPercent);
        if (controlLED) digitalWrite(LED_PIN, HIGH);
        Serial.println("[LOGIC]: Lado Local LIGADO.");
    } else {
        if (controlMotor) setMotorIntensity(drv_m2, 0); 
        if (controlLED) digitalWrite(LED_PIN, LOW); 
        Serial.println("[LOGIC]: Lado Local DESLIGADO (Repouso).");
    }
}

void handleCommand(EmdrCommandFromApp_t command) { 
    
    // 1. Prepara a configuracao temporaria (dados crus do App)
    EmdrConfigData_t incomingConfig = {
        command.durationPerSideMs,
        command.intensityPercent,
        command.command,
        command.actuatorMode
    };

    uint8_t slotId = command.profileId;
    
    // Verifica se a acao é SAVE (2)
    if (command.actionType == 2) { 
        // ------------------ ACAO: SALVAR (SAVE) ------------------
        if (slotId >= 1 && slotId <= NUM_PROFILE_SLOTS) {
            save_config_to_slot(slotId, incomingConfig);
            Serial.printf("[LOGIC]: Config SALVA no Perfil %u (Modo: %u).\n", slotId, incomingConfig.actuatorMode);
        } else {
            Serial.println("ERRO: ID de Perfil para salvar (SLOT) invalido.");
        }
        
    } else { 
        // ------------------ ACAO: EXECUTAR/CARREGAR (RUN/LOAD) ------------------
        
        bool configLoaded = false;
        
        // Tenta CARREGAR a configuracao salva se um slot válido for especificado
        if (slotId >= 1 && slotId <= NUM_PROFILE_SLOTS) {
            configLoaded = load_config_from_slot(slotId, currentConfig);
            if (configLoaded) {
                Serial.printf("[LOGIC]: Perfil %u CARREGADO da EEPROM.\n", slotId);
            }
        }
        
        // Se nao carregou da EEPROM ou o slot é inválido (e.g., slotId=0), usa a configuração que veio do App
        if (!configLoaded) {
            currentConfig = incomingConfig;
            Serial.println("[LOGIC]: Usando configuracao direta do App.");
        }

        Serial.printf(">>> EXECUTAR: Dur %lu, Int %u, Cmd %u, Modo %u\n", 
            currentConfig.durationPerSideMs, currentConfig.intensityPercent, 
             currentConfig.command, currentConfig.actuatorMode);

        // Lógica de Execução
        if (currentConfig.command == 1 && currentConfig.intensityPercent > 0) {
            isRunning = true;
            pulseDuration = currentConfig.durationPerSideMs; 
            lastToggleTime = millis(); 
            isLocalActive = true; 
            
            controlActuatorsLocal(true); 
            
            // NOVO: Enviar o primeiro comando para o Auxiliar (deve ser o oposto do Local, ou seja, DESLIGA/0)
            if (pAuxCommandCharacteristic) {
                uint8_t aux_cmd = 0; // Se Local_Active=true, Aux_Cmd=0
                pAuxCommandCharacteristic->setValue(&aux_cmd, 1);
                pAuxCommandCharacteristic->notify();
            }
        } else {
            stopStimulation();
        }
    }
}

// --- Funcao do Loop Principal ---
void run_emdr_cycle() {
    // Logica do Ciclo Sincrono (Alternancia LIGADO <-> DESLIGADO)
    if (isRunning && pulseDuration > 0) {
        if (millis() - lastToggleTime >= pulseDuration) {
            
            isLocalActive = !isLocalActive; 
            controlActuatorsLocal(isLocalActive); 
            
            // NOVO: Sincronização com o Auxiliar (Notificação)
            if (pAuxCommandCharacteristic) {
                // Se o Local ligou (isLocalActive=true), o Auxiliar deve DESLIGAR (0)
                // Se o Local desligou (isLocalActive=false), o Auxiliar deve LIGAR (1)
                uint8_t aux_cmd = isLocalActive ? 0 : 1; 
                pAuxCommandCharacteristic->setValue(&aux_cmd, 1);
                pAuxCommandCharacteristic->notify();
                Serial.printf("[LOGIC]: Sincronizacao Auxiliar: %u\n", aux_cmd);
            }
            
            lastToggleTime = millis(); 
        }
    }

    // Logica de Notificacao (atualizacao da caracteristica)
    if (pStatusCharacteristic && deviceConnected) {
        updateStatusCharacteristic();
        // O valor do contador 'value' é atualizado via global_vars
        pStatusCharacteristic->setValue(String(value++).c_str());
        pStatusCharacteristic->notify();
    }

    // Logica de Reconexao
    if (!deviceConnected && oldDeviceConnected) {
        BLEDevice::getAdvertising()->stop();
        delay(500);
        pServer->startAdvertising();
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) oldDeviceConnected = deviceConnected;
}