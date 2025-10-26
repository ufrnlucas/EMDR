// Arquivo: emdr_config.h

#pragma once

#include <Arduino.h>
#include "ble_uuids.h"
#include "data_struct.h"
#include "profile_manager.h" // Incluir este, se ainda não o fez, para a lógica de perfis
#include "global_vars.h"

// --- Variáveis de Atuadores ---
extern const uint8_t DRV_ADDR_M2;
extern Adafruit_DRV2605 drv_m2;
extern const int LED_PIN;