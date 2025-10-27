// Arquivo: ble_server.h (Versão Limpa)

#pragma once

#include "global_vars.h" // <<< NOVO: Contém todas as declarações 'extern' do BLE
#include "emdr_logic.h"  // Necessário para a assinatura de handleCommand

void init_ble_server();
// NOVO: Declaração da função utilitária de debug HEX
void debugPrintHex(const uint8_t* buffer, size_t length);