// Arquivo: motor_control.h (Versão Limpa)

#pragma once

#include "global_vars.h" // <<< NOVO: Para acessar drv_m2, LED_PIN, etc.

void setup_motor_led();
void setupDRV(Adafruit_DRV2605& drv, uint8_t addr);
void setMotorIntensity(Adafruit_DRV2605& drv, uint8_t intensity);